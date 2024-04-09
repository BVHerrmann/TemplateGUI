#pragma once

#include <QNetworkReply>
#include <QtCore>
#include "employee.h"
#include "workListItem.h"
#include "etrsdefines.h"


class ErpClient  : public QObject
{
	Q_OBJECT
public:
	ErpClient(QObject *parent);
	~ErpClient();

    void getData(const QUrl urlServerAddress, std::function<void(QJsonDocument, QString)> result);
    void postData(const QUrl urlServerAddress, const QJsonDocument postData, std::function<void(bool, QString)> result);

    void _getData(const QUrl urlServerAddress, std::function<void(QJsonDocument,QString)> result);
    void _postData(const QUrl urlServerAddress, const QJsonDocument postData, std::function<void(bool,QString)> result);
    
    void getEmployee(const QString& employeeID, std::function<void(QList<workListItem>)> result);
    void getEmployeeList(std::function<void(QList<Employee>)> result);
    void getEmployeeWorklist(const QString& employeeID, std::function<void(QList<workListItem>,QString)> result);
    void getUsers(std::function<void(QList<Employee>,QString)> result);
    void getUserStatus(const QString& cardID, std::function<void(ClockStatus,QString)> result);
    void setBookTime(const QString& cardID, const QDateTime& time, const ClockStatus& clockStatus, std::function<void(bool,QString)> result);
    void setNetworkRequestHeader(QNetworkRequest& request);
    void getProjectStatus(const QString& employeeID, std::function<void(ProjectStatusData, QString)> result);

    void setUseTestSystem(bool set) { _useTestSystem = set; }

    QUrl getUrlFromString(QString &stringUrl);

    void FillTestWorkList(QList<workListItem>& workList);
 
  private:
    int _simulateClockStatus;
    bool _useTestSystem;
    QString _testHostName;
    QString _errorTextJsonIsEmpty;
    QString _errorTextJsonNoArray;
   
   
};
