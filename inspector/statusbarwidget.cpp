#include "statusbarwidget.h"

#include <QtWidgets>

#include <colors.h>

#include "mainwindow.h"
#include "statusbarcontrolwidget.h"

#define ICON_WARNING    "q"
#define ICON_STOP       "k"
#define ICON_RESET      "R"


StatusBarWidget::StatusBarWidget(QWidget *parent) : BackgroundWidget(parent)
{
    QSettings settings;
    
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    _machine_state_button = new QPushButton(ICON_WARNING);
    _machine_state_button->setObjectName("machineState");
    connect(_machine_state_button, &QPushButton::clicked, this, &StatusBarWidget::machineStateClicked);
    layout->addWidget(_machine_state_button);
    
    _state_control = new StatusBarControlWidget(tr("Operating Mode"));
    _state_control->setProperty(kRequiredAccessLevel, settings.value(kAllowMachineState, kAccessLevelGuest).toInt());
    connect(_state_control, &StatusBarControlWidget::clicked, this, &StatusBarWidget::changeMachineStateClicked);
    layout->addWidget(_state_control);

    QWidget *bar1 = new QWidget(this);
    bar1->setObjectName("bar");
    layout->addWidget(bar1);
    
    _product_control = new StatusBarControlWidget(tr("Product"));
    _product_control->setButtonVisible(false);
    _product_control->setProperty(kRequiredAccessLevel, kAccessLevelService);
    connect(_product_control, &StatusBarControlWidget::clicked, this, &StatusBarWidget::changeProductClicked);
    layout->addWidget(_product_control);
    
    QWidget *bar2 = new QWidget(this);
    bar2->setObjectName("bar");
    layout->addWidget(bar2);
    
    _user_control = new StatusBarControlWidget(tr("User"));
    connect(_user_control, &StatusBarControlWidget::clicked, this, &StatusBarWidget::changeUserClicked);
    layout->addWidget(_user_control);
    
    _diag_state_button = new QPushButton(ICON_RESET);
    _diag_state_button->setObjectName("diagState");
    connect(_diag_state_button, &QPushButton::clicked, this, &StatusBarWidget::diagStateClicked);
    layout->addWidget(_diag_state_button);
}

void StatusBarWidget::setMachineState(const PluginInterface::MachineState &machine_state, const PluginInterface::DiagState &diag_state)
{
    QColor color = stateColor(machine_state, diag_state);
    QString color_string = QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue());
    _machine_state_button->setStyleSheet(color_string);
    _diag_state_button->setStyleSheet(color_string);
    
    _state_control->setSubtitle(stateName(machine_state));
    
    if (machine_state != PluginInterface::Off && diag_state < PluginInterface::OK) {
        _machine_state_button->setText(ICON_WARNING);
        _diag_state_button->setText(ICON_RESET);
    } else {
        _machine_state_button->setText("");
        _diag_state_button->setText("");
    }
}

void StatusBarWidget::setProduct(const QString &name)
{
    _product_control->setSubtitle(name);
}

void StatusBarWidget::setUsername(const QString &name)
{
    _user_control->setSubtitle(name);
}

void StatusBarWidget::setCanShowProductWindow(const bool canShowProductWindow)
{
    _product_control->setButtonVisible(canShowProductWindow);
}

QColor StatusBarWidget::stateColor(const PluginInterface::MachineState &machine_state, const PluginInterface::DiagState &diag_state) const
{
    switch (machine_state) {
        case PluginInterface::Off:
            return HMIColor::Grey;
            break;
        case PluginInterface::Error:
            return HMIColor::Alarm;
            break;
        default:
            break;
    }
    
    switch (diag_state) {
        case PluginInterface::WarningHigh:
            return HMIColor::WarningHigh;
            break;
        case PluginInterface::WarningLow:
            return HMIColor::WarningLow;
            break;
        case PluginInterface::OK:
            return HMIColor::OK;
            break;
        default:
            return HMIColor::Alarm;
            break;
    }
}

QString StatusBarWidget::stateName(const PluginInterface::MachineState &machine_state) const
{
    switch (machine_state) {
        case PluginInterface::Terminate:
            return tr("Terminate");
            break;
        case PluginInterface::Initializing:
            return tr("Initializing");
            break;
        case PluginInterface::EmergencyStop:
            return tr("Emergency Stop");
            break;
        case PluginInterface::Off:
            return tr("Off");
            break;
        case PluginInterface::Error:
            return tr("Error");
            break;
        case PluginInterface::Stopping:
            return tr("Stopping");
            break;
        case PluginInterface::Starting:
            return tr("Starting");
            break;
        case PluginInterface::Setup:
            return tr("Setup");
            break;
        case PluginInterface::Production:
            return tr("Production");
            break;
        default:
            return tr("Unknown");
            break;
    }
}
