#include "messagespopupdialog.h"

#include <QtWidgets>

#include <colors.h>


MessagesTableModel::MessagesTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    // initialize column headers
    _columns << tr("Date & Time") << tr("Code") << tr("Message");
}

MessagesTableModel::~MessagesTableModel()
{
}

void MessagesTableModel::updateMessages(const QList<PluginInterface::Message> &items)
{
    for (const PluginInterface::Message &item : items) {
        // check if it exists
        
        int insert_pos = INT_MAX;
        int update_pos = -1;
        for (int i=0; i < _items.count(); ++i) {
            if (insert_pos == INT_MAX && _items[i].timestamp < item.timestamp) {
                insert_pos = i;
            }
            if (_items[i].code == item.code && _items[i].plugin == item.plugin) {
                update_pos = i;
                break;
            }
        }
        
        if (update_pos == -1) {
            if (insert_pos == INT_MAX) {
                insert_pos = 0;
            }
            
            // insert
            beginInsertRows(QModelIndex(), insert_pos, insert_pos);
            _items.insert(insert_pos, item);
            endInsertRows();
        } else {
            // update
            _items[update_pos] = item;
            emit dataChanged(index(update_pos, 0), index(update_pos, 2));
        }
    }
    
    for (int i = _items.count() - 1; i >= 0; --i) {
        // check which items to remove
        bool found = false;
        for (const PluginInterface::Message &item : items) {
            if (item.code == _items[i].code) {
                found = true;
            }
        }
        if (!found) {
            beginRemoveRows(QModelIndex(), i, i);
            _items.removeAt(i);
            endRemoveRows();
        }
    }
}

QVariant MessagesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            return _columns.value(section);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant MessagesTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= _items.count())
        return QVariant();
    
    switch (role) {
    case Qt::BackgroundRole:
        {
            switch(_items.value(index.row()).type) {
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
    case Qt::DisplayRole:
        {
            switch (index.column()) {
                case 0:
                    return _items.value(index.row()).timestamp.toString("dd.MM.yy hh:mm:ss.zzz");
                    break;
                case 1:
                    return _items.value(index.row()).code;
                    break;
                case 2:
                    return _items.value(index.row()).message;
                    break;
                default:
                    return QVariant();
                    break;
            }
        }
    default:
        return QVariant();
    }
}

int MessagesTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _items.count();
}

int MessagesTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _columns.count();
}


MessagesPopupDialog::MessagesPopupDialog(PluginsController *pluginsController, QWidget *parent) : PopupDialog(parent)
{
    _pluginsController  = pluginsController;
    
    setWindowTitle(tr("Messages"));
    connect(_pluginsController, &PluginsController::machineStateChanged, this, &MessagesPopupDialog::setTitleBackground);
    setTitleBackgroundColor(stateColor(_pluginsController->machineState(), _pluginsController->diagState()));
    
    QBoxLayout *box = new QVBoxLayout();
    centralWidget()->setLayout(box);
    
    QTableView *_table_view = new QTableView(this);
    
    _model = new MessagesTableModel(this);
    connect(_pluginsController, &PluginsController::messagesChanged, _model, &MessagesTableModel::updateMessages);
    _model->updateMessages(_pluginsController->messages());
    
    _table_view->setModel(_model);
    
    _table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _table_view->setShowGrid(false);
    _table_view->setAlternatingRowColors(true);
    _table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    _table_view->setSelectionMode(QAbstractItemView::NoSelection);
    _table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _table_view->horizontalHeader()->setStretchLastSection(true);
    _table_view->horizontalHeader()->setSectionsClickable(false);
    _table_view->verticalHeader()->hide();
    
    box->addWidget(_table_view);
    
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Close);
    button_box->button(QDialogButtonBox::Reset)->setObjectName("action");
    button_box->button(QDialogButtonBox::Close)->setObjectName("close");
    connect(button_box, &QDialogButtonBox::clicked, [=](QAbstractButton *button) {
        switch (button_box->standardButton(button)) {
            case QDialogButtonBox::Reset:
                _pluginsController->resetPlugins();
                break;
            case QDialogButtonBox::Close:
                this->close();
                break;
            default:
                break;
        }
    });
    box->addWidget(button_box);
}

void MessagesPopupDialog::setTitleBackground(const PluginInterface::MachineState &machineState, const PluginInterface::DiagState &diagState)
{
    setTitleBackgroundColor(stateColor(_pluginsController->machineState(), _pluginsController->diagState()));
}

QColor MessagesPopupDialog::stateColor(const PluginInterface::MachineState &machine_state, const PluginInterface::DiagState &diag_state) const
{
    if (machine_state == PluginInterface::Off) {
        return HMIColor::Accent;
    }
    
    switch (diag_state) {
        case PluginInterface::WarningHigh:
            return HMIColor::WarningHigh;
            break;
        case PluginInterface::WarningLow:
            return HMIColor::WarningLow;
            break;
        case PluginInterface::Alarm:
            return HMIColor::Alarm;
            break;
        default:
            return HMIColor::Accent;
            break;
    }
}
