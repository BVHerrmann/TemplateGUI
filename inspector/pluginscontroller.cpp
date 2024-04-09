#include "pluginscontroller.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtSql>

#include <opencv2/core.hpp>

#include <audittrail.h>
#include <plugin.h>

#include "mainwindow.h"
#include "mainwidget.h"
#include "inspectordefines.h"
#include "inspectionresult.h"
#include "processingresult.h"
#include "translator.h"


Q_DECLARE_METATYPE(QVariant)

Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(cv::Size)
Q_DECLARE_METATYPE(cv::Rect)
Q_DECLARE_METATYPE(cv::Point)

Q_DECLARE_METATYPE(PluginInterface::MachineState)
Q_DECLARE_METATYPE(PluginInterface::DiagState)
Q_DECLARE_METATYPE(std::shared_ptr<const ProcessingResult>)


//#define DEBUG_COMMUNICATION

PluginsController::PluginsController(MainWindow *mainWindow, QObject *parent) :
    QObject(parent)
{
    registerMetaTypes();

    // alarms database
    _db = new AlarmDatabase(this);
    connect(this, &PluginsController::messagesChanged, _db, &AlarmDatabase::updateMessages);
    
    _mainWindow = mainWindow;
    _mainWindow->setPluginsController(this);

    // start out with plugins disabled
    _plugins_enabled = false;
    _requested_machine_state = PluginInterface::Off;
    _current_machine_state = PluginInterface::Initializing;
    _current_diag_state = PluginInterface::OK;
    _current_access_level = kAccessLevelGuest;
    
    // update timer
    QTimer *_update_timer = new QTimer(this);
    _update_timer->setInterval(250);
    connect(_update_timer, &QTimer::timeout, [=]() { updatePluginStates(); });
    _update_timer->start();
    
    // make sure we can close all plugins
    connect(qApp, &QApplication::aboutToQuit, this, &PluginsController::aboutToQuit);
}

PluginsController::~PluginsController()
{
    // close all threads
    while(!_plugin_threads.isEmpty()) {
        QThread *thread = _plugin_threads.takeFirst();
        thread->quit();
        thread->wait();
        delete thread;
    }
}

void PluginsController::registerMetaTypes()
{
    // qt types
    qRegisterMetaType<QVariant >();
    qRegisterMetaType<std::vector<double> >();

    // opencv types
    qRegisterMetaType<cv::Mat>();
    qRegisterMetaType<cv::Size>();
    qRegisterMetaType<cv::Rect>();
    qRegisterMetaType<cv::Point>();

    // my types
    qRegisterMetaType<PluginInterface::MachineState >();
    qRegisterMetaType<PluginInterface::DiagState >();
    qRegisterMetaType<std::shared_ptr<const InspectionResult> >();
    qRegisterMetaType<std::shared_ptr<const ProcessingResult> >();
}


void PluginsController::aboutToQuit()
{
    // tell plugins to uninitialize
    emit uninitializePlugins();

    // disable plugins
    _plugins_enabled = false;
}

void PluginsController::loadAllPlugins()
{
    QObjectList plugins = availablePlugins();

    // treat MainWindow as a normal plugin
    plugins << _mainWindow;

    // load found plugins
    for (QObject *plugin : plugins) {
        loadPlugin(plugin);
    }
    
    // plugins are all loaded. sort by priority
    std::sort(_loaded_plugins.begin(), _loaded_plugins.end(), [](const QObject *a, const QObject *b) -> bool {
        MainWindowInterface *plugin1 = qobject_cast<MainWindowInterface *>(a);
        MainWindowInterface *plugin2 = qobject_cast<MainWindowInterface *>(b);
        int prio1 = INT_MAX;
        int prio2 = INT_MAX;
        for (int i = 0; i < plugin1->mainWidgetCount(); ++i) {
            prio1 = std::min(prio1, plugin1->preferredTabIndex(i));
        }
        for (int i = 0; i < plugin2->mainWidgetCount(); ++i) {
            prio2 = std::min(prio2, plugin2->preferredTabIndex(i));
        }
        return prio1 < prio2;
    });
    
    // enable plugins
    _plugins_enabled = true;

    // tell plugins to start initialization
    emit initializePlugins();
    
    AuditTrail::message(tr("Application launched"));
}

QObjectList PluginsController::availablePlugins()
{
    // make sure this method is only run once
    if (_available_plugins.count())
        return _available_plugins;

    // static instances
    _available_plugins = QPluginLoader::staticInstances();

    // collect dynamic plugins
    QStringList pluginFiles;
#if defined(Q_OS_WIN)
    QStringList filters;
    QDir pluginsDir = QDir(qApp->applicationDirPath());
    filters << "*.dll";
    pluginsDir.setNameFilters(filters);
    pluginFiles << pluginsDir.entryList(QDir::Files);
#elif defined(Q_OS_MAC)
    QStringList filters;
    QDir pluginsDir = QDir(qApp->applicationDirPath());
    filters << "*.dylib";
    pluginsDir.setNameFilters(filters);
    if (pluginsDir.exists("../PlugIns")) {
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cd("PlugIns");
        }
    }
    pluginFiles << pluginsDir.entryList(QDir::Files);
    pluginsDir.cdUp();
    pluginsDir.cdUp();
    pluginsDir.cdUp();
    pluginFiles << pluginsDir.entryList(QDir::Files);
#endif

    // load dynamic plugins
    for (const QString &fileName : pluginFiles) {
		QString pluginFile = pluginsDir.absoluteFilePath(fileName);
		if (!fileName.startsWith("Qt5")) {
			QPluginLoader *loader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName), this);
			if (loader->load()) {
				_available_plugins << loader;
			} else {
				qWarning() << "Invalid plugin" << fileName << loader->errorString();
			}
		}
    }
    
    // load plugin translations
    Translator::getInstance()->updateTranslations();

    return _available_plugins;
}

bool PluginsController::isPluginEnabled(QObject *object)
{
    if (object == _mainWindow)
        return true;

    QString name = pluginIdentifier(object);

    if (!name.isEmpty()) {
        QSettings settings;
        return settings.value("Plugins/" + name + "/Enabled", true).toBool();
    }

    return false;
}

void PluginsController::setPluginEnabled(QObject *object, bool value)
{
    QString name = pluginIdentifier(object);
    
    if (!name.isEmpty()) {
        QString displayName = pluginName(object);
        if (value) {
            AuditTrail::message(tr("Plugin %1 enabled").arg(displayName));
        } else {
            AuditTrail::message(tr("Plugin %1 disabled").arg(displayName));
        }
        
        QSettings settings;
        settings.setValue("Plugins/" + name + "/Enabled", value);
    }
}

QString PluginsController::pluginIdentifier(QObject *object)
{
    PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
    if (plugin) {
        return plugin->identifier();
    }
    
    QPluginLoader *loader = qobject_cast<QPluginLoader *>(object);
    if (loader) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(loader->instance());
        if (plugin) {
            return plugin->identifier();
        } else if (loader->metaData().contains("className")) {
            return loader->metaData().value("className").toString();
        } else {
            QFileInfo info(loader->fileName());
            return info.fileName();
        }
    }
    
    return QString();
}

QString PluginsController::pluginName(QObject *object)
{
    PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
    if (plugin) {
        return plugin->name();
    }
    
    QPluginLoader *loader = qobject_cast<QPluginLoader *>(object);
    if (loader) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(loader->instance());
        if (plugin) {
            return plugin->name();
        } else if (loader->metaData().contains("className")) {
            return loader->metaData().value("className").toString();
        } else {
            QFileInfo info(loader->fileName());
            return info.fileName();
        }
    }
    
    return QString();
}

void PluginsController::updatePluginStates()
{
    // collect messages
    QList<PluginInterface::Message> messages;
    for (const QObject *object : _loaded_plugins) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
        if (plugin) {
            messages.append(plugin->messages());
        }
    }
    
    PluginInterface::DiagState diag_state = PluginInterface::DiagState::OK;
    if (_messages != messages) {
        // map messages type to diag state
        for (const PluginInterface::Message &message : messages) {
            // order: QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtInfoMsg
            switch (message.type) {
                case QtDebugMsg:
                case QtInfoMsg:
                    break;
                case QtFatalMsg:
                    diag_state = PluginInterface::DiagState::Alarm;
                    break;
                case QtWarningMsg:
                    diag_state = std::min(diag_state, PluginInterface::DiagState::WarningLow);
                    break;
                case QtCriticalMsg:
                    diag_state = std::min(diag_state, PluginInterface::DiagState::WarningHigh);
                    break;
            }
        }
        
        // notify of changed messages
        emit messagesChanged(messages);
        QWriteLocker locker(&_messages_lock);
        _messages = messages;
    } else {
        diag_state = _current_diag_state;
    }
        
    // get machine state
    PluginInterface::MachineState machine_state = PluginInterface::Production;
    for (const QObject *object : _loaded_plugins) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
        if (plugin && plugin->machineState() < machine_state) {
            machine_state = plugin->machineState();
        }
    }
    
    // for state off, do not show errors
    if (_requested_machine_state == PluginInterface::Off
        && (machine_state == PluginInterface::EmergencyStop || machine_state == PluginInterface::Error)) {
        machine_state = PluginInterface::Off;
    }
    
    // notify of changed machine or diag state
    if (_current_machine_state != machine_state || _current_diag_state != diag_state) {
        _current_machine_state = machine_state;
        _current_diag_state = diag_state;
        emit machineStateChanged(machine_state, diag_state);
    }
    
    // product name
    QString productName;
    bool canShowProductWindow = false;
    for (const QObject *object : _loaded_plugins) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
        if (plugin) {
            canShowProductWindow = plugin->canShowProductWindow();
            if (!plugin->currentProductName().isNull()) {
                productName = plugin->currentProductName();
                break;
            }
        }
    }
    if (_can_show_product_window != canShowProductWindow) {
        _can_show_product_window = canShowProductWindow;
        emit canShowProductWindowChanged(_can_show_product_window);
    }
    if (_current_product_name != productName) {
        _current_product_name = productName;
        emit productNameChanged(_current_product_name);
    }

    // make sure all plugins are terminated then quit
    bool allTerminated = true;
    for (const QObject* object : _loaded_plugins) {
        PluginInterface* plugin = qobject_cast<PluginInterface*>(object);
        if (plugin && plugin->machineState() != PluginInterface::Terminate) {
            allTerminated = false;
        }
    }

    if (allTerminated) {
        qApp->processEvents();
        qApp->quit();
    }
}

QString PluginsController::productName() const
{
    return _current_product_name;
}

void PluginsController::showProductWindow()
{
    for (const QObject *object : _loaded_plugins) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
        if (plugin && plugin->canShowProductWindow() && object->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("showProductWindow()")) != -1) {
            plugin->showProductWindow();
            break;
        }
    }
}

void PluginsController::loadPlugin(QObject *object)
{
    QObject *plugin = nullptr;
    QPluginLoader *loader = qobject_cast<QPluginLoader *>(object);
    if (loader) {
        plugin = loader->instance();
        if (!plugin)
            qWarning() << "Failed to create instance of plugin:" << loader->errorString();
    } else {
        plugin = object;
    }

    if (!plugin)
        return;

    if (!isPluginEnabled(plugin))
        return;
    
    qDebug() << "Load plugin:" << plugin;
    
    // check if plugin understands plugin interface
    PluginInterface *iPlugin = qobject_cast<PluginInterface *>(plugin);
    if (iPlugin) {
        _loaded_plugins << plugin;

        // move plugin to different thread
        if (plugin != _mainWindow && iPlugin->priority() != QThread::InheritPriority) {
            QThread *thread = new QThread();
            _plugin_threads << thread;
            thread->start();
            thread->setObjectName(iPlugin->name());
            thread->setPriority(iPlugin->priority());

            // attach plugin to thread
            plugin->moveToThread(thread);
            iPlugin->moveToThread(thread);

            qDebug() << "moved" << plugin << "to thread" << thread;
        }

        // connect signals
        if (plugin->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("initialize()")) != -1)
            connect(this, SIGNAL(initializePlugins()), plugin, SLOT(initialize()), Qt::QueuedConnection);
        if (plugin->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("uninitialize()")) != -1) {
            if (QThread::currentThread() == plugin->thread()) {
                connect(this, SIGNAL(uninitializePlugins()), plugin, SLOT(uninitialize()), Qt::DirectConnection);
            } else { 
                connect(this, SIGNAL(uninitializePlugins()), plugin, SLOT(uninitialize()), Qt::BlockingQueuedConnection);
            }
        }
        if (plugin->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("reset()")) != -1) {
            connect(this, SIGNAL(resetPlugins()), plugin, SLOT(reset()), Qt::QueuedConnection);
        }
        if (plugin->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("performReset()")) != -1) {
            connect(plugin, SIGNAL(performReset()), this, SIGNAL(resetPlugins()), Qt::QueuedConnection);
        }
        
        if (plugin->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("requestMachineState(const PluginInterface::MachineState)")) != -1) {
            connect(this, SIGNAL(requestedMachineStateChanged(const PluginInterface::MachineState)), plugin, SLOT(requestMachineState(const PluginInterface::MachineState)), Qt::QueuedConnection);
        }
        if (plugin->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("changeMachineState(const PluginInterface::MachineState)")) != -1) {
            connect(plugin, SIGNAL(changeMachineState(const PluginInterface::MachineState)), this, SLOT(setRequestedMachineState(const PluginInterface::MachineState)), Qt::QueuedConnection);
        }
        
        if (plugin->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("currentMachineState(const PluginInterface::MachineState, const PluginInterface::DiagState)")) != -1) {
            connect(this, SIGNAL(machineStateChanged(const PluginInterface::MachineState, const PluginInterface::DiagState)), plugin, SLOT(currentMachineState(const PluginInterface::MachineState, const PluginInterface::DiagState)), Qt::QueuedConnection);
        }

        if (plugin->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("currentUser(int, QString const&, QString const&, QString const&)")) != -1) {
            connect(this, SIGNAL(userChanged(int, QString const&, QString const&, QString const&)), plugin, SLOT(currentUser(int, QString const&, QString const&, QString const&)),
                    Qt::QueuedConnection);
        }

    } else {
        qWarning() << plugin << "is not a vaild plugin!";
        return;
    }
    
    // communication interface
    CommunicationInterface *iCommunication = qobject_cast<CommunicationInterface *>(plugin);
    if (iCommunication) {
        // plugin -> controller
        connect(plugin, SIGNAL(valuesChanged(QHash<QString, QVariant>)), this, SLOT(setValues(QHash<QString, QVariant>)));
        connect(plugin, SIGNAL(valueChanged(QString, QVariant)), this, SLOT(setValue(QString, QVariant)));

        // controller -> plugin
        connect(this, SIGNAL(valuesChanged(const QObject*,QHash<QString, QVariant>)), plugin, SLOT(setValues(const QObject*,QHash<QString, QVariant>)));
        connect(this, SIGNAL(valueChanged(const QObject*,QString, QVariant)), plugin, SLOT(setValue(const QObject*,QString, QVariant)));
    }

    // image interfaces
    ImageProducerInterface *iImageProducer = qobject_cast<ImageProducerInterface *>(plugin);
    if (iImageProducer) {
        connect(plugin, SIGNAL(newImage(QString,ulong,int,cv::Mat)), SIGNAL(newImage(QString,ulong,int,cv::Mat)), Qt::DirectConnection);
    }
    ImageConsumerInterface *iImageConsumer = qobject_cast<ImageConsumerInterface *>(plugin);
    if (iImageConsumer) {
        connect(this, SIGNAL(newImage(QString,ulong,int,cv::Mat)), plugin, SLOT(consumeImage(QString,ulong,int,cv::Mat)), Qt::DirectConnection);
    }

    // inspection result interfaces
    InspectionResultProducerInterface *iInspectionResultProducer = qobject_cast<InspectionResultProducerInterface *>(plugin);
    if (iInspectionResultProducer) {
        connect(plugin, SIGNAL(finishedInspection(std::shared_ptr<const InspectionResult>)), SIGNAL(finishedInspection(std::shared_ptr<const InspectionResult>)), Qt::QueuedConnection);
    }
    InspectionResultConsumerInterface *iInspectionResultConsumer = qobject_cast<InspectionResultConsumerInterface *>(plugin);
    if (iInspectionResultConsumer) {
        connect(this, SIGNAL(finishedInspection(std::shared_ptr<const InspectionResult>)), plugin, SLOT(consumeInspectionResult(std::shared_ptr<const InspectionResult>)));
    }
}

void PluginsController::setValues(const QHash<QString, QVariant> &values)
{
#ifdef DEBUG_COMMUNICATION
    qDebug() << QObject::sender() << values;
#endif

    if (_plugins_enabled) {
        emit valuesChanged(QObject::sender(), values);
    }
}

void PluginsController::setValue(const QString &name, const QVariant &value)
{
#ifdef DEBUG_COMMUNICATION
    qDebug() << QObject::sender() << name << value;
#endif

    if (_plugins_enabled) {
        emit valueChanged(QObject::sender(), name, value);
    }
}
