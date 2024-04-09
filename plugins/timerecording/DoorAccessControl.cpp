#include "DoorAccessControl.h"

DoorAccessControl::DoorAccessControl(BSimaticIdent* cardReader, const DEVICE device, QObject* parent) : AccessControlSystem(cardReader, device, parent)
{
    _outputSlot = device.outputModuleAddress.toUInt();
    connect(this, &AccessControlSystem::serialnumberVerified, this, &DoorAccessControl::handleAccess);
}

DoorAccessControl::~DoorAccessControl()
{
}

QHash<QString, QVariant> DoorAccessControl::profinetConfig()
{
    QHash<QString, QVariant> config;

    QVector<std::shared_ptr<PNIOOutputValue>> out_config;
    out_config << std::make_shared<PNIOOutputValue>(_q0, "DAC_Output0_" + QString::number(_outputSlot), _outputSlot, 0, 0);
    out_config << std::make_shared<PNIOOutputValue>(_q1, "DAC_Output1_" + QString::number(_outputSlot), _outputSlot, 0, 1);
    out_config << std::make_shared<PNIOOutputValue>(_q2, "DAC_Output2_" + QString::number(_outputSlot), _outputSlot, 0, 2);
    out_config << std::make_shared<PNIOOutputValue>(_q3, "DAC_Output3_" + QString::number(_outputSlot), _outputSlot, 0, 3);
    config["out_config"] = QVariant::fromValue(out_config);

    return config;
}

// main
void DoorAccessControl::handleAccess(const QString serialnumber)
{
    setOutput();
}

void DoorAccessControl::setOutput()
{
    _q0.set(true);

    if (_outputDurationTimer) {
        _outputDurationTimer->start(5000);
    } else {
        _outputDurationTimer = new QTimer;
        _outputDurationTimer->setSingleShot(true);
        connect(_outputDurationTimer, &QTimer::timeout, this, &DoorAccessControl::resetOutput);
        _outputDurationTimer->start(5000);
    }
}

void DoorAccessControl::resetOutput()
{
    _q0.set(false);
}
