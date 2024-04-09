#include "ErpClient.h"
#include "etrsdefines.h"

ErpClient::ErpClient(QObject *parent) : QObject(parent)
{
    _simulateClockStatus = 1;
    _useTestSystem = false;
    _testHostName = "?host=my351237";
    _errorTextJsonIsEmpty = tr("Error json document is empty! url:");
    _errorTextJsonNoArray = tr("Error no json array! url:");
    
}

ErpClient::~ErpClient()
{
}


void ErpClient::postData(const QUrl url, const QJsonDocument jsonDocument, std::function<void(bool, QString)> result)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);
    connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply* reply) {
        QString errorText;
        if (reply->error() != QNetworkReply::NoError) {
            errorText = tr("Error post data. ") + reply->errorString();
            qDebug() << errorText;
        }
        result(reply->error() == QNetworkReply::NoError, errorText);
    });
    QNetworkRequest request(url);
    request.setTransferTimeout(20000);
    const QByteArray postData = jsonDocument.toJson();
    manager->post(request, postData);
}

void ErpClient::getData(const QUrl url, std::function<void(QJsonDocument, QString)> result)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);
    connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply* reply) {
        QJsonDocument jsonDocument;
        QString errorText;
        if (reply->error() == QNetworkReply::NoError) {
            jsonDocument = QJsonDocument::fromJson(reply->readAll());
        } else {
            QString response(reply->readAll());
            errorText = tr("Error get data. ") + reply->errorString() + tr(" Response:%1").arg(response);
            qDebug() << errorText;
        }
        result(jsonDocument, errorText);
    });
    QNetworkRequest request(url);
    setNetworkRequestHeader(request);
    request.setTransferTimeout(20000);
    manager->get(request);
}

void ErpClient::setNetworkRequestHeader(QNetworkRequest& request)
{
    QString Authorization = "Basic ZXJwQGJlcnRyYW0uZXU6LTwubmx1MmdJait9NFohSDYpYTUxOnQrcXBlMlZCLFsvMGk9QnpsOWgjS0QuYz5ARTk=";
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", Authorization.toUtf8());
}

void ErpClient::getUsers(std::function<void(QList<Employee>,QString)> result)
{
    QString stringUrl = QString("https://erp.bertram.eu/api/timerecording/users");
    const QUrl url = getUrlFromString(stringUrl);

    getData(url, [=](QJsonDocument jsonDocument,QString errorText) {
        QList<Employee> employees;
        if (!jsonDocument.isEmpty()) {
            if (jsonDocument.isArray()) {
                QJsonArray jsonArray = jsonDocument.array();
                foreach (const QJsonValue& v, jsonArray) {
                    const QString name = v.toObject().value("name").toString();
                    const QString uuid = v.toObject().value("uuid").toString();
                    const QString id = v.toObject().value("id").toString();
                    const QString timeRecordingCardID = v.toObject().value("cardID").toString();
                    const int department = v.toObject().value("organisationalCenterID").toString().toInt();
                    const QString email = v.toObject().value("eMail").toString();
                    employees.append(Employee(QUuid(uuid), id, timeRecordingCardID, name, department, email));
                }
            } else {
                errorText = _errorTextJsonNoArray + stringUrl;
            }
        } else {
            errorText = _errorTextJsonIsEmpty + stringUrl;
        }
        result(employees,errorText);
    });
}

void ErpClient::getEmployee(const QString& employeeID, std::function<void(QList<workListItem>)> result)
{
    QString stringUrl = QString("https://erp.bertram.eu/api/employee/%1").arg(employeeID);
    const QUrl url = getUrlFromString(stringUrl);
    
}

QUrl ErpClient::getUrlFromString(QString &stringUrl)
{
    QUrl url;
    if (_useTestSystem) {
        stringUrl = stringUrl + _testHostName;
    }
    url.setUrl(stringUrl);
    return url;
}

void ErpClient::getEmployeeList(std::function<void(QList<Employee>)> result)
{
    QString stringUrl = "https://erp.bertram.eu/api/employee";
    const QUrl url = getUrlFromString(stringUrl);

    getData(url, [=](QJsonDocument jsonDocument, QString errorText) {
        QList<Employee> employees;
        if (!jsonDocument.isEmpty()) {
            if (jsonDocument.isArray()) {
                const QJsonArray jsonArray = jsonDocument.array();
                foreach (const QJsonValue& v, jsonArray) {
                    const QString name = v.toObject().value("name").toString();
                    const QString uuid = v.toObject().value("uuid").toString();
                    const QString id = v.toObject().value("id").toString();
                    const QString timeRecordingCardID = v.toObject().value("cardID").toString();
                    const int department = v.toObject().value("organisationalCenterID").toString().toInt();
                    const QString email = v.toObject().value("eMail").toString();
                    employees.append(Employee(QUuid(uuid), id, timeRecordingCardID, name, department, email));
                }
            } else {
                errorText = _errorTextJsonNoArray + stringUrl;
            }
        } else {
            errorText = _errorTextJsonIsEmpty + stringUrl;
        }
        result(employees);
    });
}

void ErpClient::getEmployeeWorklist(const QString& employeeID, std::function<void(QList<workListItem>,QString)> result)
{
    QString stringUrl = QString("https://erp.bertram.eu/api/employee/%1/worklist").arg(employeeID);
    const QUrl url = getUrlFromString(stringUrl);

    getData(url, [=](QJsonDocument jsonDocument, QString errorText) {
        QList<workListItem> workList;
        if (!jsonDocument.isEmpty()) {
            if (jsonDocument.isArray()) {
                const QJsonArray jsonArray = jsonDocument.array();
                foreach (const QJsonValue& v, jsonArray) {
                    const QString projectID = v.toObject().value("projectID").toString();
                    const QString serviceID = v.toObject().value("serviceID").toString();
                    const QString projectName = v.toObject().value("projectName").toString();
                    const QString employeeID = v.toObject().value("employeeID").toString();
                    const QString plannedPeriodStartDateTime = v.toObject().value("plannedPeriodStartDateTime").toString();
                    const QString plannedPeriodEndDateTime = v.toObject().value("plannedPeriodEndDateTime").toString();
                    const QString projectTaskName = v.toObject().value("projectTaskName").toString();
                    const bool workAreaNecessary = true;
                    workList.append(workListItem(projectID, projectName, plannedPeriodStartDateTime, plannedPeriodEndDateTime, projectTaskName, workAreaNecessary));
                }
            } else {
                errorText = _errorTextJsonNoArray + stringUrl;
            }
        } else {
            errorText = _errorTextJsonIsEmpty + stringUrl;
        }
        result(workList, errorText);
    });
}

void ErpClient::getProjectStatus(const QString& employeeID, std::function<void(ProjectStatusData, QString)> result)
{
    QString stringUrl = QString("https://erp.bertram.eu/api/employee/%1/status").arg(employeeID);
    const QUrl url = getUrlFromString(stringUrl);

    getData(url, [=](QJsonDocument jsonDocument, QString errorText) {
        ProjectStatusData projectStatusData;
        const QJsonObject obj = jsonDocument.object();
        if (jsonDocument.isObject()) {
            const QJsonObject obj = jsonDocument.object();
            projectStatusData.clockStatus = (ClockStatus)(obj["status"].toInt());
        } else {
            if (jsonDocument.isArray()) {
                const QJsonArray jsonArray = jsonDocument.array();
                for (int i = 0; i < jsonArray.count(); i++) {
                    if (i == 0) {
                        const QJsonValue value = jsonArray.at(i);
                        projectStatusData.clockStatus = (ClockStatus)(value.toObject().value("status").toInt());
                    } else {
                        if (i == 1) {
                            const QJsonValue value = jsonArray.at(i);
                            projectStatusData.startTime = value.toObject().value("startTime").toString();
                            projectStatusData.projectTaskName = value.toObject().value("projectTaskName").toString();
                            projectStatusData.projectTaskID = value.toObject().value("projectTaskID").toString();
                            projectStatusData.projectName = value.toObject().value("projectName").toString();
                            projectStatusData.date = value.toObject().value("date").toString();
                            projectStatusData.productID = value.toObject().value("productID").toString();
                            projectStatusData.projectID = value.toObject().value("projectID").toString();
                        }
                    }
                }
            } else {
                errorText = _errorTextJsonIsEmpty + stringUrl;
                qDebug() << errorText;
            }
        }
        result(projectStatusData, errorText);
    });
}

void ErpClient::getUserStatus(const QString& cardID, std::function<void(ClockStatus,QString)> result)
{
    QString stringUrl = QString("https://erp.bertram.eu/api/timerecording/%1/status").arg(cardID);
    const QUrl url = getUrlFromString(stringUrl);

    getData(url, [=](QJsonDocument jsonDocument,QString errorText) {
        ClockStatus clockStatus = NoClockStatus;
        if (jsonDocument.isObject()) {
            const QJsonObject obj = jsonDocument.object();
            clockStatus = (ClockStatus)(obj["status"].toInt());
        } else {
            errorText = _errorTextJsonIsEmpty + stringUrl;
            qDebug() << errorText;
        }
        result(clockStatus, errorText);
    });
}

void ErpClient::setBookTime(const QString& cardID, const QDateTime& bookingTime, const ClockStatus& clockStatus, std::function<void(bool,QString)> result)
{
    int ClockedInOut;
    if (clockStatus == ClockedIn) {
        ClockedInOut = ClockedOut;
    } else {
        ClockedInOut = ClockedIn;
    }
    QJsonObject obj;
    obj["action"] = ClockedInOut;
    obj["timestamp"] = bookingTime.toString("yyyy-MM-ddThh:mm:ssZ");
    
    QString stringUrl = QString("https://erp.bertram.eu/api/timerecording/%1/clock").arg(cardID);
    QUrl url = getUrlFromString(stringUrl);
    QJsonDocument doc(obj);
    postData(url, doc, result);
}







//************************************** is used for simulation ******************************
// simulate wait time/response from server
class WaitThread : public QThread
{
    void run() { msleep(5200); };
};

void ErpClient::_getData(const QUrl urlServerAddress, std::function<void(QJsonDocument, QString)> result)
{
    WaitThread* manager = new WaitThread();
    connect(manager, &QThread::finished, manager, &QThread::deleteLater);
    connect(manager, &QThread::finished, [=]() {
        QJsonObject obj;
        QString ErrorText;  //= tr("Simulierter Fehler get data");
        obj["status"] = _simulateClockStatus;
        QJsonDocument doc(obj);
        result(doc, ErrorText);
    });
    manager->start();
}

void ErpClient::_postData(const QUrl urlServerAddress, const QJsonDocument postData, std::function<void(bool, QString)> result)
{
    WaitThread* manager = new WaitThread();
    connect(manager, &QThread::finished, manager, &QThread::deleteLater);
    connect(manager, &QThread::finished, [=]() {
        const QJsonObject obj = postData.object();
        QString ErrorText;  //= tr("Simulierter Fehler post data");
        _simulateClockStatus = (ClockStatus)(obj["action"].toInt());

        result(true, ErrorText);
    });
    manager->start();
}

void ErpClient::FillTestWorkList(QList<workListItem>& workList)
{
    QJsonObject obj;

    obj["projectID"] = "P23456";
    obj["projectName"] = "Innenverguetung";
    obj["employeeID"] = "44";
    obj["plannedPeriodStartDateTime"] = "2023-01-12T18:45:22Z";
    obj["plannedPeriodEndDateTime"] = "2023-02-23T13:34:34Z";
    obj["projectTaskName"] = "Glas";
    obj["workArea"] = true;

    QJsonArray jsonArray;

    jsonArray.append(obj);

    foreach (const QJsonValue& v, jsonArray) {
        const QString projectID = v.toObject().value("projectID").toString();
        const QString projectName = v.toObject().value("projectName").toString();
        const QString employeeID = v.toObject().value("employeeID").toString();
        const QString plannedPeriodStartDateTime = v.toObject().value("plannedPeriodStartDateTime").toString();
        const QString plannedPeriodEndDateTime = v.toObject().value("plannedPeriodEndDateTime").toString();
        const QString projectTaskName = v.toObject().value("projectTaskName").toString();
        const bool workAreaNecessary = v.toObject().value("workArea").toBool();
        workList.append(workListItem(projectID, projectName, plannedPeriodStartDateTime, plannedPeriodEndDateTime, projectTaskName, workAreaNecessary));
    }
}



