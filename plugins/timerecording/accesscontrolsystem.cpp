#include "accesscontrolsystem.h"
#include "etrsplugin.h"

AccessControlSystem::AccessControlSystem(BSimaticIdent* cardReader, const DEVICE device, QObject* parent) : QObject(parent)
{
    _device = device;
    _cardReader = cardReader;
    /* if (cardReader) {
        _cardReader = cardReader;
    } else {
        qFatal("DoorAccessControl: No CardReader Found");
    }
    */
    _employeeList = &EmployeeList::instance();
    connect(_employeeList, &EmployeeList::employeesUpdated, this, &AccessControlSystem::updateListOfEmployee);
    updateListOfEmployee();
}

AccessControlSystem::~AccessControlSystem()
{
    if (_cardReader) {
        delete _cardReader;
    }
}

bool AccessControlSystem::isCardReaderConnected()
{
    if (_cardReader) {
        return _cardReader->isConnected();
    } else {
        return true;
    }
}

QString AccessControlSystem::UUID() const
{
    return _device.UUID;
}

QString AccessControlSystem::deviceConnection() const
{
    return _device.connection;
}

// main
void AccessControlSystem::handleLogin(const QString serialnumber)
{
    qDebug() << "Transponder-ID:" << serialnumber;

    if (_cardReader) {
        if (verifySerialnumber(serialnumber)) {
            _cardReader->setLed(BSimaticIdent::green, 400);
            emit serialnumberVerified(serialnumber);
        } else {
            _cardReader->setLed(BSimaticIdent::red, 1000);
        }
    } else {
        if (verifySerialnumber(serialnumber)) {
            emit serialnumberVerified(serialnumber);
        } 
    }
}

QString AccessControlSystem::getNameByCardID(const QString& cardId)
{
   return _employees.value(cardId).formattedName();
}

QString AccessControlSystem::getEmployeesIDByCardID(const QString& cardId)
{
    return _employees.value(cardId).id();
}

void AccessControlSystem::updateListOfEmployee()
{
    QWriteLocker locker(&_readWriteLock);

    QHash<QString, Employee> employeesTemp = _employeeList->employees(_device.authorizedDepartments, _device.authorizedUsers);

    if (!employeesTemp.isEmpty()) {
        _employees = employeesTemp;
    }
}

bool AccessControlSystem::verifySerialnumber(const QString& serialnumber)
{
    return _employees.contains(serialnumber);
}