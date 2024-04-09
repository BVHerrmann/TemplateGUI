#include "devicemanagement.h"

#include <QDebug>

DeviceManagement::DeviceManagement(QObject* parent) : QObject(parent)
{
    _devices = loadDevices();
}

DeviceManagement::~DeviceManagement()
{
}

void DeviceManagement::addDevice(const DEVICE device)
{
    QList<DEVICE> devices = _devices.values();
    devices.append(device);
    saveDevices(devices);
}

void DeviceManagement::deleteDevice(const QString uuid)
{
    _devices.remove(uuid);
    saveDevices(_devices.values());
}

QList<DEVICE> DeviceManagement::devices() const
{
    return _devices.values();
}

QHash<QString, DEVICE> DeviceManagement::loadDevices()
{
    QHash<QString, DEVICE> devices;
    QSettings settings;

    int size = settings.beginReadArray(ketrsDevice);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        DEVICE device;
        device.UUID = settings.value("uuid").toString();
        device.name = settings.value("name").toString();
        device.systemType = settings.value("systemType").toString();
        device.inputAddress = settings.value("inputAddress").toString();
        device.connection = settings.value("deviceConnection").toString();
        device.outputModuleAddress = settings.value("outputModuleAddress").toString();
        device.authorizedDepartments = settings.value("departments").toList();
        device.authorizedUsers = settings.value("authorizedUsers").toList();

        devices.insert(device.UUID, device);
    }
    settings.endArray();

    return devices;
}

void DeviceManagement::saveDevices(const QList<DEVICE> devices)
{
    QSettings settings;
    settings.remove(ketrsDevice);

    settings.beginWriteArray(ketrsDevice);
    for (int i = 0; i < devices.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("uuid", devices.at(i).UUID);
        settings.setValue("name", devices.at(i).name);
        settings.setValue("systemType", devices.at(i).systemType);
        settings.setValue("deviceConnection", devices.at(i).connection);
        settings.setValue("inputAddress", devices.at(i).inputAddress);
        settings.setValue("outputModuleAddress", devices.at(i).outputModuleAddress);
        settings.setValue("departments", devices.at(i).authorizedDepartments);
        settings.setValue("authorizedUsers", devices.at(i).authorizedUsers);
    }
    settings.endArray();
}
