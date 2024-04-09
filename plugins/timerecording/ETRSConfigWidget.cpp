#include "ETRSConfigWidget.h"

#include "QNetworkInterface"
#include "QSerialPortInfo"
#include "devicepopupdialog.h"

ETRSConfigWidget::ETRSConfigWidget(ETRSPlugin* plugin, QWidget* parent) : QWidget(parent)
{
    _plugin = plugin;
    setupUi();
}

ETRSConfigWidget::~ETRSConfigWidget()
{
}

void ETRSConfigWidget::setDevice()
{
    DevicePopupDialog m(this);
    bool ok = m.exec();
    DEVICE device = m.device();

    if (ok) {
        DeviceManagement().addDevice(device);
    }
}

void ETRSConfigWidget::deleteDevice(const DEVICE device, QWidget* deviceWidget)
{
    deviceWidget->deleteLater();
}

QList<QVariant> ETRSConfigWidget::getDepartmentNumbers(const QString data)
{
    QList<QVariant> departments;
    QRegularExpression rx("[, ]");
    QStringList depList = data.split(rx, QString::SkipEmptyParts);
    for (int i = 0; i < depList.size(); i++) {
        if (!depList.at(i).isEmpty()) {
            departments.append(depList.at(i).toInt());
        }
    }
    return departments;
}

void ETRSConfigWidget::setupUi()
{
    QSettings* settings = new QSettings();
    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);

    // Hardware Configuration
    QGroupBox* hwConf_group = new QGroupBox(tr("Hardware Configuration"));
    QFormLayout* hwConf_layout = new QFormLayout();
    hwConf_group->setLayout(hwConf_layout);
    layout->addWidget(hwConf_group);

    QComboBox* selectNetInt = new QComboBox();

    // add Network Interfaces
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    for (auto networkInterface : list) {
        if (networkInterface.type() == QNetworkInterface::Ethernet) {
            selectNetInt->addItem(networkInterface.humanReadableName(), networkInterface.hardwareAddress());
        }
    }

    connect(selectNetInt, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { settings->setValue(kPNDriverNetworkInterface, selectNetInt->itemData(index)); });
    const QString mac = settings->value(kPNDriverNetworkInterface).toString();
    if (!mac.isEmpty()) {
        selectNetInt->setCurrentIndex(selectNetInt->findData(mac));
    } else {
        settings->setValue(kPNDriverNetworkInterface, selectNetInt->itemData(selectNetInt->currentIndex()));
    }

    hwConf_layout->addRow(new QLabel(tr("Network Interface")), selectNetInt);

    QLabel* hw_path = new QLabel(settings->value(kPNDriverHardwareConfigurationPath).toString());
    QPushButton* selectHwConf = new QPushButton(tr("Select Hardware Configuration"), this);
    connect(selectHwConf, &QPushButton::pressed, [=]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Open Hardware Configuration"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("XML Files (*.xml)"));
        if (!path.isEmpty()) {
            settings->setValue(kPNDriverHardwareConfigurationPath, path);
            hw_path->setText(path);
        }
    });
    hwConf_layout->addRow(new QLabel(tr("Configuration")), selectHwConf);
    hwConf_layout->addRow(new QLabel(tr("Path: ")), hw_path);

    // Devices
    QGroupBox* device_group = new QGroupBox(tr("Devices"));
    QFormLayout* device_layout = new QFormLayout();
    device_group->setLayout(device_layout);
    layout->addWidget(device_group);

    QScrollArea* device_area = new QScrollArea();
    device_area->setWidgetResizable(true);
    device_area->setFrameStyle(QFrame::Plain);
    device_layout->addWidget(device_area);

    QWidget* area_widget = new QWidget();
    QFormLayout* area_layout = new QFormLayout();
    area_widget->setLayout(area_layout);
    device_area->setWidget(area_widget);

    refreshDeviceListing(area_widget);

    QPushButton* addDevice = new QPushButton(tr("Add Device"), this);
    connect(addDevice, &QPushButton::pressed, [=]() {
        setDevice();
        refreshDeviceListing(area_widget);
    });
    device_layout->addRow(addDevice);
}

void ETRSConfigWidget::refreshDeviceListing(QWidget* widget)
{
    qDeleteAll(widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly));

    QList<DEVICE> devices = DeviceManagement().devices();
    std::sort(devices.begin(), devices.end(), [](const DEVICE& a, const DEVICE& b) -> bool { return a.name < b.name; });

    for (int i = 0; i < devices.size(); i++) {
        ETRSDeviceWidget* deviceWidget = new ETRSDeviceWidget(devices.at(i), widget);
        connect(deviceWidget, &ETRSDeviceWidget::deleteWidget, this, &ETRSConfigWidget::deleteDevice);
        widget->layout()->addWidget(deviceWidget);
    }
}