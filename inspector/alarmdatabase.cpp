#include "alarmdatabase.h"

#include <colors.h>


AlarmModel::AlarmModel(QObject *parent, QSqlDatabase db) : QSqlTableModel(parent, db)
{
    setTable("alarms");
    
    setHeaderData(0, Qt::Horizontal, tr("Date & Time"));
    setHeaderData(1, Qt::Horizontal, tr("Typ"));
    setHeaderData(2, Qt::Horizontal, tr("Code"));
    setHeaderData(3, Qt::Horizontal, tr("Plugin"));
    setHeaderData(4, Qt::Horizontal, tr("Message"));
    
    setSort(0, Qt::DescendingOrder);
}

QVariant AlarmModel::data(const QModelIndex &idx, int role) const
{
    if (role == Qt::BackgroundRole) {
        QtMsgType type = (QtMsgType)QSqlTableModel::data(index(idx.row(), 1)).toLongLong(); // 1: type
        switch(type) {
        case QtWarningMsg:
            return HMIColor::WarningLowTranslucent;
            break;
        case QtCriticalMsg:
            return HMIColor::WarningHighTranslucent;
            break;
        case QtFatalMsg:
            return HMIColor::AlarmTranslucent;
            break;
        default:
            return QVariant();
        }
    }
    else if (role == Qt::DisplayRole && idx.column() == 0) { // start
        QDateTime ts = QDateTime::fromString(QSqlTableModel::data(idx, role).toString(), Qt::ISODateWithMs);
        return QLocale::system().toString(ts.date(), QLocale::ShortFormat) + " " + ts.time().toString(Qt::ISODateWithMs);
    }
    
    return QSqlTableModel::data(idx, role);
}

AlarmDatabase::AlarmDatabase(QObject *parent) : QObject(parent)
{
    _database = QSqlDatabase::addDatabase("QSQLITE");
    _database.setDatabaseName(databasePath());
    
    if (!_database.open()) {
        qCritical() << "Failed to open Alarms Database:" << _database.lastError();
    }
    
    createAlarmsTable();
    cleanupAlarmsTable();
    cleanupDatabase();
}

AlarmDatabase::~AlarmDatabase()
{
    // end all open alarms
    updateMessages(QList<PluginInterface::Message>());
}

QString AlarmDatabase::databasePath() const
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    if (!dir.exists()) {
        bool result = dir.mkpath(dir.path());
        if (!result) {
            qWarning() << "Failed to create directory at:" << dir.path();
        }
    }
    
    return dir.filePath("db.sqlite");
}

AlarmModel *AlarmDatabase::tableModel()
{
    return new AlarmModel(this, _database);
}

void AlarmDatabase::updateMessages(const QList<PluginInterface::Message> &items)
{
    // new alarms
    for (const auto &message : items) {
        if (message.type != QtInfoMsg && message.type && !_messages.contains(message)) {
            QSqlQuery query;
            query.prepare("INSERT INTO alarms (timestamp, type, code, plugin, message) VALUES (:timestamp, :type, :code, :plugin, :message)");
            query.bindValue(":timestamp", message.timestamp);
            query.bindValue(":type", message.type);
            query.bindValue(":code", message.code);
            query.bindValue(":plugin", message.plugin);
            query.bindValue(":message", message.message);
            
            if (!query.exec()) {
                qWarning() << "Failed to store alarm:" << query.lastError();
            }
        }
    }
    
    // removed alarms
    for (const auto &message : _messages) {
        if (message.type != QtInfoMsg && message.type != QtDebugMsg && !items.contains(message)) {
            QSqlQuery query;
            query.prepare("INSERT INTO alarms (timestamp, type, code, plugin, message) VALUES (:timestamp, :type, :code, :plugin, :message)");
            query.bindValue(":timestamp", QDateTime::currentDateTime());
            query.bindValue(":type", QtDebugMsg); // going message
            query.bindValue(":code", message.code);
            query.bindValue(":plugin", message.plugin);
            query.bindValue(":message", message.message);
            
            if (!query.exec()) {
                qWarning() << "Failed to store alarm:" << query.lastError();
            }
        }
    }
    
    _messages = items;
}

void AlarmDatabase::createAlarmsTable()
{
    QSqlQuery query("CREATE TABLE IF NOT EXISTS alarms (timestamp DATETIME NOT NULL, type INTEGER, code INTEGER NOT NULL, plugin TEXT NOT NULL, message TEXTNOT NULL, PRIMARY KEY (timestamp, code, plugin))", _database);
    if (!query.isActive()) {
        qWarning() << "Failed to create Table: " << query.lastError();
    }
}

void AlarmDatabase::cleanupAlarmsTable()
{
    QSqlQuery query;
    query.prepare("DELETE FROM alarms WHERE timestamp < :timestamp");
    query.bindValue(":timestamp", QDateTime::currentDateTime().addYears(-1));
    
    if (!query.exec()) {
        qWarning() << "Failed to remove old entries: " << query.lastError();
    }
}

void AlarmDatabase::cleanupDatabase()
{
    QSqlQuery query("VACUUM", _database);
    if (!query.isActive()) {
        qWarning() << "Failed to create Table: " << query.lastError();
    }
}
