#ifndef MESSAGESPOPUPDIALOG_H
#define MESSAGESPOPUPDIALOG_H

#include <popupdialog.h>

#include <interfaces.h>
#include <QtWidgets>

#include "pluginscontroller.h"


class MessagesTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MessagesTableModel(QObject *parent = 0);
    virtual ~MessagesTableModel();
    
    // QAbstractTableModel
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    
public slots:
    void updateMessages(const QList<PluginInterface::Message> &items);
    
protected:
    QList<QString> _columns;
    QList<PluginInterface::Message> _items;
};


class MessagesPopupDialog : public PopupDialog
{
    Q_OBJECT
public:
    explicit MessagesPopupDialog(PluginsController *pluginsController, QWidget *parent = nullptr);
    
signals:
    
public slots:
    void setTitleBackground(const PluginInterface::MachineState &machineState, const PluginInterface::DiagState &diagState);
    
protected:
    QColor stateColor(const PluginInterface::MachineState &machine_state, const PluginInterface::DiagState &diag_state) const;
    
    PluginsController *_pluginsController;
    MessagesTableModel *_model;
};

#endif // MESSAGESPOPUPDIALOG_H
