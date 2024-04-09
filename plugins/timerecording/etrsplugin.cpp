#include "etrsplugin.h"

#include <logic.h>
#include <pniovalue.h>

#include <QtCore>
#include <QtGui>

#include "ETRSConfigWidget.h"
#include "etrswidget.h"
#include "ProjectWidget.h"


ETRSPlugin::ETRSPlugin(QObject* parent) : Plugin(parent)
{
     QSettings settings;
    _etrsWidget = NULL;
    _useTestSystem = false;
    _showStandardView = true;
    _error_mask = noErr;
    _warning_mask = noWarn;
    _switchToStandardView.setInterval(5000);
    _switchToStandardView.setSingleShot(true);

    // QString name = qgetenv("USER");
    //name = qgetenv("USERNAME");

    //QProcessEnvironmentenv = QProcessEnvironment::systemEnviroment();
    //QString username = env.value("USER");
    

    if (!settings.contains(kUseTestSystem)) {
        settings.setValue(kUseTestSystem, _useTestSystem);
    }
    _useTestSystem = settings.value(kUseTestSystem, false).toBool();
    if (!settings.contains(kWorkingTimeRecordingVariante)) {
        settings.setValue(kWorkingTimeRecordingVariante, kVarianteWorkingTime);
    }
    _timeRecordingVariante = settings.value(kWorkingTimeRecordingVariante, kVarianteWorkingTime).toInt();
    //_timeRecordingVariante = kVarianteWorkingTime;
    _etrsWidget = new ETRSWidget(this);
    _config_widget = new ETRSConfigWidget(this);
    EmployeeList* employeeList = &EmployeeList::instance();
    employeeList->setPlugin(this);
    employeeList->updateListOfUsers();

    connect(employeeList, &EmployeeList::employeesUpdated, this, [this]() {
        auto* employeeList = &EmployeeList::instance();
        auto employees = employeeList->employees();

        auto user_array = QJsonArray();
        for (auto employee : employees) {
            auto user_object = QJsonObject();
            user_object.insert("uuid", employee.uuid().toString());
            user_object.insert("name", employee.formattedName());
            user_object.insert("access_level", kAccessLevelUser);
            user_object.insert("password", employee.id());
            user_object.insert("card_id", employee.timeRecordingCardID());

            user_array.append(user_object);
        }
        emit valueChanged("updateUsers", user_array);
    });
}

ETRSPlugin::~ETRSPlugin()
{
}

void ETRSPlugin::reset()
{
    clearMessages();
    _warning_mask = 0;
}

void ETRSPlugin::setAccessLevelGuest()
{
    emit valueChanged("AccessLevel", kAccessLevelGuest);
}

const ETRSPlugin::MachineState ETRSPlugin::machineState() const
{
    if (_request_state == Terminate) {
        return Terminate;
    }
    if (_activeSystems.isEmpty()) {
        return Initializing;
    }

    for (int i = 0; i < _activeSystems.size(); i++) {
        if (!_activeSystems.at(i)->isCardReaderConnected()) {
            return Initializing;
        }
    }

    return Production;
}

const ETRSPlugin::DiagState ETRSPlugin::diagState() const
{
    if (_error_mask != 0) {
        return Alarm;
    }
    if (_warning_mask != 0) {
        return WarningLow;
    }
    return OK;

    // WarningHigh
}

void ETRSPlugin::setWarningMask(int set,const QString &warningText)
{
    _warning_mask |= set;
    _warningText = warningText;
}

void ETRSPlugin::updateMessages()
{
    QList<PluginInterface::Message> messages;

    if (_warning_mask & warnNoSerialPortFound) {
        messages << PluginInterface::Message(20000 | warnNoSerialPortFound, tr("No SerialPort found"), QtWarningMsg);
    }
    if (_warning_mask & warnNoCardReaderFound) {
        messages << PluginInterface::Message(20000 | warnNoCardReaderFound, tr("No Cardreader found"), QtWarningMsg);
    }
    if (_warning_mask & warnWrongDeviceConnection) {
        messages << PluginInterface::Message(20000 | warnWrongDeviceConnection, tr("Wrong device Connection"), QtWarningMsg);
    }
    if (_warning_mask & warnWrongSystemType) {
        messages << PluginInterface::Message(20000 | warnWrongSystemType, tr("Wrong Systemtype"), QtWarningMsg);
    }
    if (_warning_mask & warnNoHardwareConfigurtaion) {
        messages << PluginInterface::Message(30000 | warnNoHardwareConfigurtaion, tr("Hardware configuration was not found"), QtWarningMsg);
    }
    if (_warning_mask & warnNoMacAddress) {
        messages << PluginInterface::Message(30000 | warnNoMacAddress, tr("MAC Address was not found"), QtWarningMsg);
    }
    if (_warning_mask & warnMacAddressNotParsed) {
        messages << PluginInterface::Message(30000 | warnMacAddressNotParsed, tr("MAC address could not be parsed"), QtWarningMsg);
    }
    if (_warning_mask & warnServerRequestError) {
        messages << PluginInterface::Message(30000 | warnServerRequestError, _warningText, QtWarningMsg);
    }
    PluginInterface::updateMessages(messages);
}

const ETRSPlugin::WidgetType ETRSPlugin::widgetType(const int idx) const
{
    switch (idx) {
        case 0:
            return Application;
            break;
        case 1:
            return Settings;
            break;
        
        default:
            return Application;
    }
}

const QString ETRSPlugin::title(const int idx) const
{
    switch (idx) {
        case 0:
            return tr("Zeiterfassung");
            break;
        case 1:
            return tr("Configuration");
            break;
      
        default:
            return name();
    }
}

QWidget* ETRSPlugin::mainWidget(const int idx) const
{
    switch (idx) {
        case 0:
             if (_etrsWidget) {
                 _etrsWidget->setShowStandardView(_showStandardView, _currentEmployeeID, _currentEmployeeName);
             }
             return _etrsWidget;
        case 1:
             return _config_widget;
        default:
            return nullptr;
    }
}

int ETRSPlugin::requiredWidgetAccessLevel(const int idx) const
{
    switch (idx) {
        case 0:
            return kAccessLevelGuest;
        case 1:
            return kAccessLevelSysOp;
        default:
            return kAccessLevelBertram;
    }
}

void ETRSPlugin::initialize()
{
    qDebug() << "Initialize ETRS Plugin";

    QSettings settings;

    if (settings.value("Plugins/PNDriver/Enabled", true).toBool()) {
        QHash<QString, QVariant> config;
        config["type"] = "controller";

        QFile file(settings.value(kPNDriverHardwareConfigurationPath).toString());
        if (!file.open(QIODevice::ReadOnly)) {
            _warning_mask |= warnNoHardwareConfigurtaion;
            qWarning("Hardware configuration was not found");
        }
        config["config"] = file.readAll();  // Hardware - Config
        file.close();

        QString mac = settings.value(kPNDriverNetworkInterface).toString();
        if (mac.isEmpty()) {
            _warning_mask |= warnNoMacAddress;
            qWarning("MAC Address was not found");
            return;
        }

        QByteArray mac_bytes;
        bool ok;
        for (auto part : mac.split(":")) {
            mac_bytes.append(part.toUInt(&ok, 16));
            if (!ok) {
                _warning_mask |= warnMacAddressNotParsed;
                qWarning("MAC address could not be parsed");
                return;
            }
        }

        config["network_interface_mac"] = mac_bytes;

        emit valueChanged("PNIO/configure", config);

    } else {
        QList<DEVICE> devices = DeviceManagement().devices();
        for (DEVICE device : devices) {
            setupDevice(device);
        }
    }
    emit valueChanged("updateUi", QVariant());  // ruft dann mainWidget(const int idx) auf
    
}

void ETRSPlugin::uninitialize()
{
}

QHash<QString, QVariant> ETRSPlugin::setupDevice(const DEVICE device)
{
    QHash<QString, QVariant> device_config({{"in_config", QVariant::fromValue(QVector<std::shared_ptr<PNIOInputValue>>())},
                                            {"out_config", QVariant::fromValue(QVector<std::shared_ptr<PNIOOutputValue>>())},
                                            {"record_config", QVariant::fromValue(QVector<std::shared_ptr<PNIORecord>>())}});

    QIODevice* CardReaderSerialPort = nullptr;
    QThread* thread;

    if (device.connection == "UserManagement") {
        TimeRecordingSystem* timeRecordingSystem = new TimeRecordingSystem(NULL, device);
        timeRecordingSystem->setPlugin(this);
               
        connect(this, &ETRSPlugin::setCurrentUser, timeRecordingSystem, &TimeRecordingSystem::handleLogin);
        connect(timeRecordingSystem, &TimeRecordingSystem::login, _etrsWidget, &ETRSWidget::handleLogin);
        connect(timeRecordingSystem, &TimeRecordingSystem::logout, _etrsWidget, &ETRSWidget::handleLogout);
        connect(timeRecordingSystem, &TimeRecordingSystem::log, _etrsWidget, &ETRSWidget::handleLog);
        _activeSystems.append(timeRecordingSystem);
    } else if (device.connection == "USB") {
        QString adress = device.inputAddress;
        QSerialPortInfo serialPortInfo = getSerialPortCardreader(adress);
        if (!serialPortInfo.isNull()) {
            CardReaderSerialPort = new QSerialPort(serialPortInfo.portName());
        } else {
            qWarning("No SerialPort found");
            _warning_mask |= warnNoSerialPortFound;
        }
    } else if (device.connection == "Profinet") {
        BPNSerialPort* serialPort = new BPNSerialPort(device.inputAddress.toUInt());
        connect(this, &ETRSPlugin::setValuesReady, serialPort, &BPNSerialPort::setValues);

        CardReaderSerialPort = serialPort;

        device_config["in_config"] = QVariant::fromValue(device_config["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>() +
                                                         serialPort->profinetConfig()["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>());
        device_config["out_config"] = QVariant::fromValue(device_config["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>() +
                                                          serialPort->profinetConfig()["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>());
        device_config["record_config"] = QVariant::fromValue(device_config["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>() +
                                                             serialPort->profinetConfig()["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>());

        serialPort->open(QIODevice::ReadWrite);
    } else {
        qWarning("Wrong device connection");
        _warning_mask |= warnWrongDeviceConnection;
    }

    if (CardReaderSerialPort) {
        BSimaticIdent* cardReader = new BSimaticIdent;

        thread = new QThread();
        thread->start();
        thread->setObjectName(device.UUID);
        thread->setPriority(QThread::IdlePriority);

        CardReaderSerialPort->moveToThread(thread);
        cardReader->moveToThread(thread);
        QMetaObject::invokeMethod(cardReader, "setDevice", Q_ARG(QIODevice*, CardReaderSerialPort));

        if (device.systemType == "TimeRecordingSystem") {
            TimeRecordingSystem* timeRecordingSystem = new TimeRecordingSystem(cardReader, device);
            timeRecordingSystem->moveToThread(thread);
            connect(cardReader, &BSimaticIdent::login, timeRecordingSystem, &TimeRecordingSystem::handleLogin);
            connect(timeRecordingSystem, &TimeRecordingSystem::login, _etrsWidget, &ETRSWidget::handleLogin);
            connect(timeRecordingSystem, &TimeRecordingSystem::logout, _etrsWidget, &ETRSWidget::handleLogout);
            connect(timeRecordingSystem, &TimeRecordingSystem::log, _etrsWidget, &ETRSWidget::handleLog);
            _activeSystems.append(timeRecordingSystem);
        } else if (device.systemType == "DoorAccessControl") {
            DoorAccessControl* doorAccessControl = new DoorAccessControl(cardReader, device);
            doorAccessControl->moveToThread(thread);
            connect(cardReader, &BSimaticIdent::login, doorAccessControl, &DoorAccessControl::handleLogin);

            device_config["in_config"] = QVariant::fromValue(device_config["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>() +
                                                             doorAccessControl->profinetConfig()["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>());
            device_config["out_config"] = QVariant::fromValue(device_config["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>() +
                                                              doorAccessControl->profinetConfig()["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>());
            device_config["record_config"] = QVariant::fromValue(device_config["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>() +
                                                                 doorAccessControl->profinetConfig()["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>());

            _activeSystems.append(doorAccessControl);
        } else {
            qWarning("Wrong System Type");
            _warning_mask |= warnWrongSystemType;
        }
    } else {
        if (device.connection != "UserManagement") {
            qWarning("No CardReader found");
            _warning_mask |= warnNoCardReaderFound;
        }
    }

    return device_config;
}

QSerialPortInfo ETRSPlugin::getSerialPortCardreader(const QString name)
{
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& port : availablePorts) {
        if (port.portName() == name) {
            return port;
        }
    }
    return QSerialPortInfo();
}

void ETRSPlugin::setupConfig()
{
    QHash<QString, QVariant> config({{"in_config", QVariant::fromValue(QVector<std::shared_ptr<PNIOInputValue>>())},
                                     {"out_config", QVariant::fromValue(QVector<std::shared_ptr<PNIOOutputValue>>())},
                                     {"record_config", QVariant::fromValue(QVector<std::shared_ptr<PNIORecord>>())}});

    QList<DEVICE> devices = DeviceManagement().devices();
    for (DEVICE device : devices) {
        auto device_config = setupDevice(device);
        config["in_config"] = QVariant::fromValue(config["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>() + device_config["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>());
        config["out_config"] =
            QVariant::fromValue(config["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>() + device_config["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>());
        config["record_config"] =
            QVariant::fromValue(config["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>() + device_config["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>());
    }

    emit valueChanged("PNIO/setup", config);
}

void ETRSPlugin::currentUser(int access_level, QString const& username, QString const& uuid, QString const& card_id)
{
    qDebug() << username << uuid << card_id;
    if (card_id.isEmpty() ) {
        setShowStandardView(true, QString(), QString());
    } else {
        emit setCurrentUser(card_id);
    }
}

void ETRSPlugin::ReStartTimerShowProjectView()
{
    emit restartTimerShowProjectView();
}

void ETRSPlugin::setShowStandardView(bool set, const QString& employeeID, const QString& employeeName)
{
    _showStandardView = set;
    _currentEmployeeID = employeeID;
    _currentEmployeeName = employeeName;
    emit valueChanged("updateUi", QVariant());  // ruft dann mainWidget(const int idx) auf
}

void ETRSPlugin::setValues(const QHash<QString, QVariant>& values)
{
    QHash<QString, QVariant>::const_iterator i = values.constBegin();
    while (i != values.constEnd()) {
        if (i.key() == "PNIOControllerConfig") {
            setupConfig();
        }
        ++i;
    }
    emit setValuesReady(values);
}
