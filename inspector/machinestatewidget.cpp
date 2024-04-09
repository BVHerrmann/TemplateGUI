#include "machinestatewidget.h"


MachineStateWidget::MachineStateWidget(PluginsController* pluginsController, QWidget *parent) :
    QWidget(parent)
{
    _pluginsController = pluginsController;

    setupUi();
}

void MachineStateWidget::setupUi()
{    
    QTableView* _table_view = new QTableView(this);

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

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(_table_view, 0, 0);

    setLayout(layout);
}
