#ifndef MACHINESTATEWIDGET_H
#define MACHINESTATEWIDGET_H

#include <QtGui>
#include <QtWidgets>

#include "pluginscontroller.h"

#include "messagespopupdialog.h"


class MachineStateWidget : public QWidget
{
Q_OBJECT
public:
    explicit MachineStateWidget(PluginsController* pluginsController, QWidget *parent = 0);

private:
    void setupUi();

    PluginsController* _pluginsController;
    MessagesTableModel* _model;
};

#endif // MACHINESTATEWIDGET_H
