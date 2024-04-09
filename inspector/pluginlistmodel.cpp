#include "pluginlistmodel.h"

#include <QtCore>

#include "interfaces.h"
#include "pluginscontroller.h"


static bool sortHelper(const QObject *o1, const QObject *o2)
{
    MainWindowInterface *m1 = qobject_cast<MainWindowInterface *>(o1);
    MainWindowInterface *m2 = qobject_cast<MainWindowInterface *>(o2);

    if (!m1 && !m2) {
        return false;
    } else if (!m1) {
        return false;
    } else if (!m2) {
        return true;
    } else {
        return m1->preferredTabIndex(0) < m2->preferredTabIndex(0);
    }
}


PluginListModel::PluginListModel(PluginsController *pluginsController, QObject *parent) :
    QAbstractListModel(parent)
{
    // get available plugins from plugin controller
    _pluginsController = pluginsController;
    _plugins = pluginsController->availablePlugins();
    std::sort(_plugins.begin(), _plugins.end(), sortHelper);
}

void PluginListModel::toggleItemAtIndex(const QModelIndex &index)
{
    QObject *object = _plugins.at(index.row());

    bool status = _pluginsController->isPluginEnabled(object);
    _pluginsController->setPluginEnabled(object, !status);

    emit dataChanged(index, index);
}

Qt::ItemFlags PluginListModel::flags(const QModelIndex &index) const
{
    (void)index;
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

QVariant PluginListModel::data(const QModelIndex &index, int role) const
{
    QObject *object = _plugins.at(index.row());
    QVariant value;

    switch (role)
    {
    case Qt::DisplayRole:
    {
        value = _pluginsController->pluginName(object);
        break;
    }
    case Qt::CheckStateRole:
    {
        if (_pluginsController->isPluginEnabled(object))
            value = Qt::Checked;
        else
            value = Qt::Unchecked;
        break;
    }
    }

    return value;
}

int PluginListModel::rowCount(const QModelIndex &parent) const
{
    (void)parent;

    return _plugins.count();
}
