#ifndef TIMERECORDINGSYSTEM_H
#define TIMERECORDINGSYSTEM_H

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QtConcurrent>

#include "BSimaticident.h"
#include "EmployeeList.h"
#include "accesscontrolsystem.h"
#include "employee.h"
#include "erpclient.h"
#include "etrsplugin.h"



typedef struct EmployeeTimeEvent_ {
    QString cardId = QString();
    QDateTime bookingTime = QDateTime();
    ClockStatus clockStatus = NoClockStatus;
} EmployeeTimeEvent;

QDataStream& operator<<(QDataStream& stream, const EmployeeTimeEvent& timeEvent);

QDataStream& operator>>(QDataStream& stream, EmployeeTimeEvent& timeEvent);

class TimeRecordingSystem : public AccessControlSystem
{
    Q_OBJECT

  public:
    TimeRecordingSystem(BSimaticIdent* cardReader, const DEVICE device, QObject* parent = nullptr);
    ~TimeRecordingSystem();

    void setPlugin(ETRSPlugin* plugin);
    void ShowProjectList(const QString cardId);
    
   
   
  signals:
    void log(const QString employeeName, const QDateTime time, const QString message);
    void login(const QString employeeName, const QDateTime time);
    void logout(const QString employeeName, const QDateTime time);
    void setCurrentUser(const QString card_id);
    void signalTest();
    

  public slots:
    void bookNextTimeEvent();
    void handleTimeEvent(const QString serialnumber);
    void slotSetWarningMessage(const QString &);
    //void slotShowStandardView();
    //void slotRestartTimerShowProjectView();

  private:
    void bookTime(EmployeeTimeEvent timeEvent, std::function<void(bool,QString)> result);
    void bookTimeRequest(const QString& serialnumber, const QDateTime& timestamp);
    void loadEmployeeTimeEvents();
    void saveEmployeeTimeEvents();
    void setUpPath();
    void setUpTimer();
    
    BSimaticIdent* _cardReader = nullptr;
    EmployeeList* _employeeList = nullptr;
    QString _path = QString();
    QReadWriteLock _bookTimeLock;
    QReadWriteLock _saveLock;
    QTimer _bookEmployeeTimeEvent;
    ErpClient* _erpClient = nullptr;
    ETRSPlugin* _plugin = nullptr;
    //QTimer* _timerShowStandardView = nullptr;
    
    QList<EmployeeTimeEvent> _employeeTimeEvents;
    EmployeeTimeEvent _timeBookingWaitingList;
};
#endif  // TIMERECORDINGSYSTEM_H
