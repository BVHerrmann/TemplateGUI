#ifndef ALARMDATABASE_H
#define ALARMDATABASE_H

#include <QtSql>

#include <interfaces.h>


class AlarmModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit AlarmModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase());

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const;
};


class AlarmDatabase : public QObject
{
    Q_OBJECT
public:
    explicit AlarmDatabase(QObject *parent = nullptr);
    ~AlarmDatabase();
    
    AlarmModel *tableModel();
    
signals:
    
public slots:
    void updateMessages(const QList<PluginInterface::Message> &items);
    
private:
    QSqlDatabase _database;
    
    QList<PluginInterface::Message> _messages;
    
    QString databasePath() const;
    
    void createAlarmsTable();
    void cleanupAlarmsTable();
    void cleanupDatabase();
};

#endif // ALARMDATABASE_H
