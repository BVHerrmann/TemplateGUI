#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <QDataStream>
#include <QObject>
#include <QUuid>
#include <QtDebug>

class Employee
{
  public:
    Employee();
    Employee(const QUuid uuid, const QString id, const QString timeRecordingCardID, const QString formattedName, const int department,const QString email);
    ~Employee();

    // getter
    QUuid uuid() const { return _uuid; }
    QString id() const { return _id; }
    QString timeRecordingCardID() const { return _timeRecordingCardID; }
    QString formattedName() const { return _formattedName; }
    QString email() const { return _email; }
    int department() const { return _department; }

    // setter
    void setDepartment(const int departmnet) { _department = departmnet; }

    bool isEqual(const Employee& Employee) const;

  private:
    QUuid _uuid = QUuid();
    QString _id = QString();
    QString _timeRecordingCardID = QString();
    QString _formattedName = QString();
    QString _email = QString();
    int _department = 0;
   
};

QDebug operator<<(QDebug debug, const Employee& employee);

QDataStream& operator<<(QDataStream& stream, const Employee& employee);
QDataStream& operator>>(QDataStream& stream, Employee& employee);

bool operator==(const Employee& a, const Employee& b);

#endif  // EMPLOYEE_H
