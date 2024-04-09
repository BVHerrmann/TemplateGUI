#include "timerecordingsystem.h"
#include "erpclient.h"

QDataStream& operator<<(QDataStream& stream, const EmployeeTimeEvent& timeEvent)
{
    stream << timeEvent.cardId << timeEvent.bookingTime << timeEvent.clockStatus;
    return stream;
}

QDataStream& operator>>(QDataStream& stream, EmployeeTimeEvent& timeEvent)
{
    stream >> timeEvent.cardId >> timeEvent.bookingTime >> timeEvent.clockStatus;
    return stream;
}

TimeRecordingSystem::TimeRecordingSystem(BSimaticIdent* cardReader, const DEVICE device, QObject* parent) : AccessControlSystem(cardReader, device, parent)
{
    connect(this, &AccessControlSystem::serialnumberVerified, this, &TimeRecordingSystem::handleTimeEvent);
    setUpPath();
    setUpTimer();
    loadEmployeeTimeEvents();
    _erpClient = new ErpClient(this);
}

TimeRecordingSystem::~TimeRecordingSystem()
{
    
}

void TimeRecordingSystem::setPlugin(ETRSPlugin* plugin)
{
    _plugin = plugin;
    if (_plugin && _erpClient) {
        _plugin->useTestSystem();
        _erpClient->setUseTestSystem(_plugin->useTestSystem());
    }
}

void TimeRecordingSystem::slotSetWarningMessage(const QString &warning)
{
    if (_plugin) {
        _plugin->setWarningMask(warnServerRequestError, warning);
    }
}

void TimeRecordingSystem::handleTimeEvent(const QString cardId)
{
    if (!cardId.isEmpty()) {
        if (_plugin->getTimeRecordingVariante() == kVarianteWorkingTime) {
            if (!(_timeBookingWaitingList.cardId == cardId && _timeBookingWaitingList.bookingTime.addSecs(5) > QDateTime::currentDateTime())) {
                bookTimeRequest(cardId, QDateTime::currentDateTime());
            } else {
                emit log("", QDateTime::currentDateTime(), "Buchung ist noch nicht abgeschlossen.");
            }
        } else {
            ShowProjectList(cardId);
        }
    }
}

void TimeRecordingSystem::ShowProjectList(const QString cardId)
{
    QString EmployeeID   = getEmployeesIDByCardID(cardId);
    QString EmployeeName = getNameByCardID(cardId);
    if (_plugin) {
        _plugin->setShowStandardView(false, EmployeeID, EmployeeName);
    }
}

void TimeRecordingSystem::bookTime(EmployeeTimeEvent timeEvent, std::function<void(bool,QString)> result)
{
    if (timeEvent.clockStatus == NoClockStatus) {
        _erpClient->getUserStatus(timeEvent.cardId, [=](ClockStatus status,QString ErrorText) {
          _erpClient->setBookTime(timeEvent.cardId, timeEvent.bookingTime, status, result);
        });
    } else {
        _erpClient->setBookTime(timeEvent.cardId, timeEvent.bookingTime, timeEvent.clockStatus, result);
    }
}

void TimeRecordingSystem::bookNextTimeEvent()
{
    QReadLocker lock(&_bookTimeLock);

    if (!_employeeTimeEvents.isEmpty()) {
        auto timeEvent = _employeeTimeEvents.first();
        bookTime(timeEvent, [=](bool result,QString ErrorText) {
            if (result) {
                QWriteLocker lock(&_bookTimeLock);
                _employeeTimeEvents.removeFirst();
                saveEmployeeTimeEvents();
            } else {
                slotSetWarningMessage(ErrorText);
            }
        });
    }
}

void TimeRecordingSystem::bookTimeRequest(const QString& serialnumber, const QDateTime& timestamp)
{
    auto bookingDateTime = timestamp;
    // So that employees who are there shortly before 6 o'clock do not have to wait
    if (bookingDateTime.time() < QTime(6, 0) && bookingDateTime.time() >= QTime(5, 30)) {
        bookingDateTime.setTime(QTime(6, 0));
    }
    // create Employee Time Event
    EmployeeTimeEvent timeEvent;
    timeEvent.cardId = serialnumber;
    timeEvent.bookingTime = bookingDateTime;

    QString const employeeName = _employees.value(timeEvent.cardId).formattedName();
    // time bookings are allowed between 6:00 - 20:00
    if (bookingDateTime.time() < QTime(6, 0, 0) || bookingDateTime.time() > QTime(20, 0, 0)) {
        emit log(employeeName, timeEvent.bookingTime, "Buchungen in der Zeit von 20:00 - 6:00 Uhr sind nicht möglich!");
        return;
    }
    emit log(employeeName, timeEvent.bookingTime, "");

    _timeBookingWaitingList = timeEvent;

    _bookTimeLock.lockForWrite();
    _employeeTimeEvents.append(timeEvent);
    _bookTimeLock.unlock();
    
    saveEmployeeTimeEvents();
    _erpClient->getUserStatus(timeEvent.cardId, [=](ClockStatus status,QString ErrorText) {
        if (status != NoClockStatus) {
            if (status == ClockedIn) {
                qDebug().nospace() << "Benutzer: " << employeeName << " (" << serialnumber << ") Arbeitsende";
                emit logout(employeeName, timeEvent.bookingTime);
            } else {
                qDebug().nospace() << "Benutzer: " << employeeName << " (" << serialnumber << ") Arbeitsbeginn";
                emit login(employeeName, timeEvent.bookingTime);
            }
        } else {
            qDebug().nospace() << "Benutzer: " << employeeName << " (" << serialnumber << ") Buchung gespeichert. Keine Verbindung zum Server vorhanden";
            emit log(employeeName, timeEvent.bookingTime,"Buchung gespeichert. Keine Verbindung zum Server vorhanden");
            qWarning() << __FUNCTION__ << "No Connection";
            slotSetWarningMessage(ErrorText);
        }
    });
}

void TimeRecordingSystem::loadEmployeeTimeEvents()
{
    QString path = _path + "EmployeeTimeEvents";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        in >> _employeeTimeEvents;
    }
    file.close();
}

void TimeRecordingSystem::setUpTimer()
{
    _bookEmployeeTimeEvent.setTimerType(Qt::VeryCoarseTimer);
    connect(&_bookEmployeeTimeEvent, &QTimer::timeout, [=]() { bookNextTimeEvent(); });
    _bookEmployeeTimeEvent.start(1000);
}

void TimeRecordingSystem::saveEmployeeTimeEvents()
{
    QReadLocker locker(&_saveLock);

    QString path = _path + "EmployeeTimeEvents";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return;
    QDataStream out(&file);
    out << _employeeTimeEvents;
    file.close();
}

void TimeRecordingSystem::setUpPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(path);
    if (!dir.exists()) dir.mkpath(path);

    _path = dir.absolutePath() + "/";
}
