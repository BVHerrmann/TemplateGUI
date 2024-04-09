#include "machinestatepopupdialog.h"

#include <QtWidgets>

#include <audittrail.h>


MachineStatePopupDialog::MachineStatePopupDialog(PluginsController *pluginsController, QWidget *parent) : PopupDialog(parent)
{
    _pluginsController  = pluginsController;
    
    setWindowTitle(tr("Operating Mode"));
    
    QBoxLayout *box = new QVBoxLayout();
    centralWidget()->setLayout(box);
    
    _button_group = new QButtonGroup(this);
    _button_group->setExclusive(true);
    
    QPushButton *off_button = new QPushButton(tr("Off"));
    off_button->setCheckable(true);
    off_button->setObjectName("action");
    connect(off_button, &QPushButton::clicked, [=]() {
        if (_pluginsController->requestedMachineState() != PluginInterface::Off) {
            AuditTrail::message(QObject::tr("Changed Machine State to %1").arg(tr("Off")));
        }
        _pluginsController->setRequestedMachineState(PluginInterface::Off);
        close();
    });
    _button_group->addButton(off_button, PluginInterface::Off);
    box->addWidget(off_button);

    QPushButton *setup_button = new QPushButton(tr("Setup"));
    setup_button->setCheckable(true);
    setup_button->setObjectName("action");
    connect(setup_button, &QPushButton::clicked, [=]() {
        if (_pluginsController->requestedMachineState() != PluginInterface::Setup) {
            AuditTrail::message(QObject::tr("Changed Machine State to %1").arg(tr("Setup")));
        }
        _pluginsController->setRequestedMachineState(PluginInterface::Setup);
        close();
    });
    _button_group->addButton(setup_button, PluginInterface::Setup);
    box->addWidget(setup_button);
    
    QPushButton *production_button = new QPushButton(tr("Production"));
    production_button->setCheckable(true);
    production_button->setObjectName("action");
    connect(production_button, &QPushButton::clicked, [=]() {
        if (_pluginsController->requestedMachineState() != PluginInterface::Production) {
            AuditTrail::message(QObject::tr("Changed Machine State to %1").arg(tr("Production")));
        }
        _pluginsController->setRequestedMachineState(PluginInterface::Production);
        close();
    });
    _button_group->addButton(production_button, PluginInterface::Production);
    box->addWidget(production_button);
    
    box->addSpacing(16);
  
    QPushButton *close_button = new QPushButton(tr("Close"));
    close_button->setObjectName("close");
    connect(close_button, &QPushButton::clicked, [=]() {
        close();
    });
    box->addWidget(close_button);

    updateUi();
    
    QTimer *update_timer = new QTimer(this);
    update_timer->setInterval(250);
    connect(update_timer, &QTimer::timeout, [=]() { updateUi(); });
    update_timer->start();
}

void MachineStatePopupDialog::updateUi()
{
    // enable / disable
    switch(_pluginsController->machineState()) {
        case PluginInterface::Initializing:
            _button_group->button(PluginInterface::Off)->setEnabled(true);
            _button_group->button(PluginInterface::Setup)->setEnabled(false);
            _button_group->button(PluginInterface::Production)->setEnabled(false);
            break;
        default:
            _button_group->button(PluginInterface::Off)->setEnabled(true);
            _button_group->button(PluginInterface::Setup)->setEnabled(true);
            _button_group->button(PluginInterface::Production)->setEnabled(true);
            break;
    }
    
    // checked
    switch(_pluginsController->requestedMachineState()) {
        case PluginInterface::Terminate:
        case PluginInterface::Initializing:
        case PluginInterface::Off:
        case PluginInterface::Stopping:
            _button_group->button(PluginInterface::Off)->setChecked(true);
            break;
        case PluginInterface::Setup:
            _button_group->button(PluginInterface::Setup)->setChecked(true);
            break;
        case PluginInterface::Production:
        case PluginInterface::Starting:
        case PluginInterface::Error:
        case PluginInterface::EmergencyStop:
            _button_group->button(PluginInterface::Production)->setChecked(true);
            break;
    }
}
