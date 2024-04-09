#ifndef MACHINESTATEPOPUPDIALOG_H
#define MACHINESTATEPOPUPDIALOG_H

#include <popupdialog.h>

#include <interfaces.h>
#include <QtWidgets>

#include "pluginscontroller.h"


class MachineStatePopupDialog : public PopupDialog
{
    Q_OBJECT
public:
    explicit MachineStatePopupDialog(PluginsController *pluginsController, QWidget *parent = nullptr);
    
signals:
    
public slots:
    void updateUi();
    
protected:
    PluginsController *_pluginsController;
    
    QButtonGroup *_button_group;
};

#endif // MACHINESTATEPOPUPDIALOG_H
