#include "employee.h"

Employee::Employee()
{
}

Employee::Employee(const QUuid uuid, const QString id, const QString timeRecordingCardID, const QString formattedName, const int department, const QString email)
{
    _uuid = uuid;
    _id = id;
    _timeRecordingCardID = timeRecordingCardID;
    _formattedName = formattedName;
    _department = department;
    _email = email;
}

Employee::~Employee()
{
}

bool Employee::isEqual(const Employee& Employee) const
{
    if (_uuid == Employee.uuid() && _id == Employee.id() && _timeRecordingCardID == Employee.timeRecordingCardID() && formattedName() == Employee.formattedName() &&
        Employee.department() == _department) {
        return true;
    }
    return false;
}

QDebug operator<<(QDebug debug, const Employee& employee)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << employee.uuid() << ", " << employee.id() << ", " << employee.timeRecordingCardID() << ", " << employee.formattedName() << ", " << employee.department() << ')' << "\n";

    return debug;
}

QDataStream& operator<<(QDataStream& stream, const Employee& employee)
{
    stream << employee.uuid().toString(QUuid::WithoutBraces);
    stream << employee.id();
    stream << employee.timeRecordingCardID();
    stream << employee.formattedName();
    stream << employee.department();
    stream << employee.email();

    return stream;
}

QDataStream& operator>>(QDataStream& stream, Employee& employee)
{
    QString uuid;
    QString id;
    QString timeRecordingCardID;
    QString formattedName;
    QString email;
    int department;

    stream >> uuid;
    stream >> id;
    stream >> timeRecordingCardID;
    stream >> formattedName;
    stream >> department;
    stream >> email;
    employee = Employee(QUuid(uuid), id, timeRecordingCardID, formattedName, department, email);

    return stream;
}

bool operator==(const Employee& a, const Employee& b)
{
    return a.isEqual(b);
}
