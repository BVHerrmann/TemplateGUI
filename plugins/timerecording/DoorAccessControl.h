#ifndef DOORACCESSCONTROL_H
#define DOORACCESSCONTROL_H

#include <logic.h>
#include <pniovalue.h>

#include <QMutex>
#include <QObject>
#include <QtConcurrent>

#include "BSimaticident.h"
#include "EmployeeList.h"
#include "accesscontrolsystem.h"
#include "employee.h"

class DoorAccessControl : public AccessControlSystem
{
    Q_OBJECT

  public:
    DoorAccessControl(BSimaticIdent* cardReader, const DEVICE device, QObject* parent = nullptr);
    ~DoorAccessControl();
    QHash<QString, QVariant> profinetConfig() override;

  public slots:
    void handleAccess(const QString serialnumber);

  private slots:
    void resetOutput();

  private:
    void setOutput();

    QTimer* _outputDurationTimer = nullptr;
    uint32_t _outputSlot;

    output<bool> _q0;
    output<bool> _q1;
    output<bool> _q2;
    output<bool> _q3;
};

#endif  // DOORACCESSCONTROL_H
