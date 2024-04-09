#ifndef ACCESSCONTROLSYSTEM_H
#define ACCESSCONTROLSYSTEM_H

#include <logic.h>
#include <pniovalue.h>

#include <QMutex>
#include <QObject>
#include <QtConcurrent>

#include "BSimaticident.h"
#include "EmployeeList.h"
#include "devicemanagement.h"
#include "employee.h"

class AccessControlSystem : public QObject
{
    Q_OBJECT

  public:
    AccessControlSystem(BSimaticIdent* cardReader, const DEVICE device, QObject* parent = nullptr);
    ~AccessControlSystem();
    bool isCardReaderConnected();
    QString UUID() const;
    QString deviceConnection() const;
    virtual QHash<QString, QVariant> profinetConfig() { return QHash<QString, QVariant>(); }  // return an empty config
    QString getNameByCardID(const QString& cardId);
    QString getEmployeesIDByCardID(const QString& cardId);
    

  signals:
    void serialnumberVerified(const QString serialnumber);

  public slots:
    void handleLogin(const QString serialnumber);

  private slots:
    void updateListOfEmployee();

  protected:
    QHash<QString, Employee> _employees;

  private:
    // functions
    bool verifySerialnumber(const QString& serialnumber);

    // variables
    BSimaticIdent* _cardReader = nullptr;
    EmployeeList* _employeeList = nullptr;
    QReadWriteLock _readWriteLock;
    DEVICE _device;
};

#endif  // ACCESSCONTROLSYSTEM_H
