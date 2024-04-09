#ifndef EMPLOYEELIST_H
#define EMPLOYEELIST_H

#include <QtCore>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QReadWriteLock>

#include "employee.h"

class ErpClient;
class ETRSPlugin;
class EmployeeList : public QObject
{
    Q_OBJECT

  public:
    static EmployeeList& instance(void)
    {
        static EmployeeList instance;
        return instance;
    }
    void setPlugin(ETRSPlugin* plugin);
    QHash<QString, Employee> employees();
    QHash<QString, Employee> employees(const QList<QVariant> departments, const QList<QVariant> authorizedUsers);
    void updateListEmployees(const QJsonDocument&);
    void loadWorkList(const QString employeeID);
   
  signals:
    void employeesUpdated();

  public slots:
    void updateListOfUsers();
    void slotSetWarningMessage(const QString&);
  private:
    EmployeeList();
    ~EmployeeList() = default;
    EmployeeList(const EmployeeList&) = delete;
    EmployeeList& operator=(const EmployeeList&) = delete;

    void loadListOfEmployee();
    void setUpPath();
    void setUpTimer();
 
    // Variable
    QString _path;
    QTimer _update;
    QTimer _timerTimeoutNetworkReply;
    QReadWriteLock _saveLoadLock;
    QReadWriteLock _readWriteLock;
    QList<Employee> _employees;
    ErpClient* _erpClient = nullptr;
    ETRSPlugin* _plugin;
};

#endif  // EMPLOYEELIST_H
