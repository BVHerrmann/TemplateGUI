#include "EmployeeList.h"
#include <QDebug>
#include <QtNetwork>
#include "etrsplugin.h"
#include "erpclient.h"

EmployeeList::EmployeeList()
{
    _plugin = nullptr;
    _erpClient = nullptr;
    setUpPath();
    setUpTimer();

    loadListOfEmployee();
    _erpClient = new ErpClient(this);
}

void EmployeeList::setPlugin(ETRSPlugin* plugin)
{
    _plugin = plugin;
    if (_plugin && _erpClient) {
        _plugin->useTestSystem();
        _erpClient->setUseTestSystem(_plugin->useTestSystem());
    }
}

void EmployeeList::updateListOfUsers()
{
    _erpClient->getUsers([=](QList<Employee> employees,QString errorText) { 
          if (!employees.isEmpty() && employees != _employees) {
            _employees = employees;
            QWriteLocker locker(&_saveLoadLock);
            QString path = _path + "Employees";
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                QDataStream out(&file);
                out << _employees;
            }
            file.close();
            emit employeesUpdated();
            qDebug() << "Employees data loaded successfully";
        } 
        if (employees.isEmpty()) {
            slotSetWarningMessage(errorText);
        }
        //loadWorkList(QString("44"));
    });
}

void EmployeeList::slotSetWarningMessage(const QString &warning)
{
    if (_plugin) {
        _plugin->setWarningMask(warnServerRequestError, warning);
    }
}

void EmployeeList::loadListOfEmployee()
{
    QWriteLocker locker(&_saveLoadLock);
    QString path = _path + "Employees";
    
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        in >> _employees;
    }
    file.close();
}

void EmployeeList::setUpTimer()
{
    _update.setTimerType(Qt::VeryCoarseTimer);
    int hourInSec = 3600;
    _update.start(hourInSec * 1000);
    connect(&_update, &QTimer::timeout, this, &EmployeeList::updateListOfUsers);
}

void EmployeeList::setUpPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(path);
    if (!dir.exists()) dir.mkpath(path);
      _path = dir.absolutePath() + "/";
}

void EmployeeList::loadWorkList(const QString employeeID)
{
    _erpClient->getEmployeeWorklist(employeeID, [=](QList<workListItem> worklist,QString ErrorMasg) { 
        if (worklist.count() == 0) {
            qWarning() << tr("Work list is empty");
        }
    });
}

QHash<QString, Employee> EmployeeList::employees()
{
    QReadLocker locker(&_readWriteLock);
    QHash<QString, Employee> employees;
    QList<Employee>::const_iterator i;
    for (i = _employees.constBegin(); i != _employees.constEnd(); ++i) {
        employees.insert(i->timeRecordingCardID(), *i);
    }
    return employees;
}

QHash<QString, Employee> EmployeeList::employees(const QList<QVariant> departments, const QList<QVariant> authorizedUsers)
{
    QReadLocker locker(&_readWriteLock);
    QHash<QString, Employee> employees;
    QList<Employee>::const_iterator i;
    for (i = _employees.constBegin(); i != _employees.constEnd(); ++i) {
        if (departments.contains(i->department())) {
            employees.insert(i->timeRecordingCardID(), *i);
            continue;
        }
        if (authorizedUsers.contains(i->id())) {
            employees.insert(i->timeRecordingCardID(), *i);
            continue;
        }
    }
    return employees;
}