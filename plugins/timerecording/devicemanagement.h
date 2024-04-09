#ifndef DEVICEMANAGMENT_H
#define DEVICEMANAGMENT_H

#include <QObject>
#include <QSettings>

#include "etrsdefines.h"

typedef struct {
    QString UUID;
    QString name;
    QString systemType;
    QString connection;
    QString inputAddress;
    QString outputModuleAddress;
    QList<QVariant> authorizedDepartments;
    QList<QVariant> authorizedUsers;
} DEVICE;

class DeviceManagement : public QObject
{
    Q_OBJECT

  public:
    DeviceManagement(QObject* parent = nullptr);
    ~DeviceManagement();

    void addDevice(const DEVICE device);
    void deleteDevice(const QString uuid);
    QList<DEVICE> devices() const;

  private:
    QHash<QString, DEVICE> loadDevices();
    void saveDevices(const QList<DEVICE> device);

    QHash<QString, DEVICE> _devices;
};

#endif  // DEVICEMANAGMENT_H
