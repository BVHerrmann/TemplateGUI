#include "ETRSDeviceWidget.h"

ETRSDeviceWidget::ETRSDeviceWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

ETRSDeviceWidget::ETRSDeviceWidget(const DEVICE device, QWidget* parent) : QWidget(parent)
{
    _device = device;

    setupUi();
}

ETRSDeviceWidget::~ETRSDeviceWidget()
{
}

void ETRSDeviceWidget::setupUi()
{
    QGridLayout* mainLayout = new QGridLayout();
    setLayout(mainLayout);

    QString departments;
    for (int i = 0; i < _device.authorizedDepartments.size(); i++) {
        departments.append(_device.authorizedDepartments.at(i).toString());
        if (i != _device.authorizedDepartments.size() - 1) {
            departments.append(", ");
        }
    }

    QString authorizedUsers;
    for (int i = 0; i < _device.authorizedUsers.size(); i++) {
        authorizedUsers.append(_device.authorizedUsers.at(i).toString());
        if (i != _device.authorizedUsers.size() - 1) {
            authorizedUsers.append(", ");
        }
    }

    mainLayout->setRowMinimumHeight(0, 30);
    mainLayout->setRowMinimumHeight(1, 30);
    mainLayout->setRowMinimumHeight(2, 30);

    mainLayout->setColumnMinimumWidth(0, 300);
    mainLayout->setColumnMinimumWidth(2, 100);

    mainLayout->addWidget(new QLabel(QString("Name: " + _device.name), this), 0, 0);
    mainLayout->addWidget(new QLabel(QString("System Type: " + _device.systemType), this), 1, 0);
    mainLayout->addWidget(new QLabel(QString("Device Connection: " + _device.connection), this), 2, 0);
    mainLayout->addWidget(new QLabel(QString("Authorized Departments: " + departments), this), 0, 1);
    mainLayout->addWidget(new QLabel(QString("Authorized Users: " + authorizedUsers), this), 1, 1);
    mainLayout->addWidget(new QLabel(QString("Adress: " + _device.inputAddress), this), 2, 1);
    if (_device.systemType == "DoorAccessControl") {
        mainLayout->addWidget(new QLabel(QString("OutputAdress: " + _device.outputModuleAddress), this), 3, 1);
    }

    QPushButton* button = new QPushButton("Delete");
    button->setObjectName("delete");
    connect(button, &QPushButton::pressed, [=]() {
        DeviceManagement().deleteDevice(_device.UUID);
        emit deleteWidget(_device, this);
    });
    mainLayout->addWidget(button, 2, 2);
}
