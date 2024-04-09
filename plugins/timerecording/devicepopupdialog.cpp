#include "devicepopupdialog.h"

#include <QSerialPortInfo>
#include <QtWidgets>

DevicePopupDialog::DevicePopupDialog(QWidget* parent) : PopupDialog(parent)
{
    _device.UUID = QUuid::createUuid().toString();

    setWindowTitle(tr("Add Device"));

    QGridLayout* grid = new QGridLayout();
    centralWidget()->setLayout(grid);

    grid->setRowMinimumHeight(0, 16);

    // Name
    grid->addWidget(new QLabel("Name:"), 1, 0);
    QLineEdit* nameInput = new QLineEdit();
    connect(nameInput, &QLineEdit::textChanged, [=](const QString& value) { _device.name = value; });
    grid->addWidget(nameInput, 1, 1);

    // Connection
    grid->addWidget(new QLabel("Device connection:"), 2, 0);
    QComboBox* deviceConnection = new QComboBox();
    deviceConnection->addItem("USB");
    deviceConnection->addItem("Profinet");
    deviceConnection->addItem("UserManagement");
    connect(deviceConnection, &QComboBox::currentTextChanged, [=](const QString& value) { _device.connection = value; });
    _device.connection = deviceConnection->currentText();
    grid->addWidget(deviceConnection, 2, 1);

    // System type
    grid->addWidget(new QLabel("System type:"), 3, 0);
    QComboBox* systemType = new QComboBox();
    systemType->addItem("DoorAccessControl");
    systemType->addItem("TimeRecordingSystem");
    connect(systemType, &QComboBox::currentTextChanged, [=](const QString& value) { _device.systemType = value; });
    _device.systemType = systemType->currentText();
    grid->addWidget(systemType, 3, 1);

    // device Addess
    grid->addWidget(new QLabel("Device address:"), 4, 0);
    QLineEdit* adressInput = new QLineEdit();
    adressInput->setValidator(new QRegExpValidator(QRegExp("[0-9]*")));
    connect(adressInput, &QLineEdit::textChanged, [=](const QString& value) { _device.inputAddress = value; });
    grid->addWidget(adressInput, 4, 1);
    adressInput->hide();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    QComboBox* ComPort = new QComboBox();
    for (int i = 0; i < ports.size(); i++) {
        ComPort->addItem(ports.at(i).portName());
    }
    connect(ComPort, &QComboBox::currentTextChanged, [=](const QString& value) { _device.inputAddress = value; });
    _device.inputAddress = ComPort->currentText();
    grid->addWidget(ComPort, 4, 1);

    // Hardware Adress
    QLabel* outputAdressLabel = new QLabel("OutputAdress:");
    grid->addWidget(outputAdressLabel, 5, 0);

    QLineEdit* outputAdressInput = new QLineEdit();
    outputAdressInput->setValidator(new QRegExpValidator(QRegExp("[0-9]*")));
    connect(outputAdressInput, &QLineEdit::textChanged, [=](const QString& value) { _device.outputModuleAddress = value; });
    grid->addWidget(outputAdressInput, 5, 1);

    // Department
    grid->addWidget(new QLabel("Department numbers:"), 6, 0);
    QRegExp depInputFilter("[0-9, ]*");
    QLineEdit* departmentInput = new QLineEdit();
    departmentInput->setValidator(new QRegExpValidator(depInputFilter, departmentInput));
    connect(departmentInput, &QLineEdit::textChanged, [=](const QString& value) { _device.authorizedDepartments = getDepartmentNumbers(value); });
    grid->addWidget(departmentInput, 6, 1);

    connect(deviceConnection, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), [=](const QString& text) {
        if (text == "Profinet") {
            ComPort->hide();
            adressInput->show();
        } else {
            if (text == "USB") {
                adressInput->hide();
                ComPort->show();
            } else {
                if (text == "UserManagement") {
                    adressInput->hide();
                    ComPort->show();
                    systemType->setCurrentIndex(1);
                }
            }
        }
    });

    connect(systemType, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), [=](const QString& text) {
        if (text == "DoorAccessControl") {
            outputAdressLabel->show();
            outputAdressInput->show();
        } else {
            outputAdressInput->hide();
            outputAdressLabel->hide();
        }
    });

    grid->setRowMinimumHeight(7, 16);

    // buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    grid->addWidget(buttonBox, 8, 1);
}

QList<QVariant> DevicePopupDialog::getDepartmentNumbers(const QString data)
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