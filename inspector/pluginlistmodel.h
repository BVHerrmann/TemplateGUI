#ifndef PLUGINLISTMODEL_H
#define PLUGINLISTMODEL_H

#include <QAbstractListModel>

class PluginsController;


class PluginListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PluginListModel(PluginsController *pluginsController, QObject *parent = 0);
    
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    
public slots:
    void toggleItemAtIndex(const QModelIndex &index);
    
private:
    PluginsController *_pluginsController;
    QObjectList _plugins;
};

#endif // PLUGINLISTMODEL_H
