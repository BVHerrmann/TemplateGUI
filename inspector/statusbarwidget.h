#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include "backgroundwidget.h"

#include <QColor>
class QPushButton;

#include "interfaces.h"
class StatusBarControlWidget;


class StatusBarWidget : public BackgroundWidget
{
    Q_OBJECT
public:
    explicit StatusBarWidget(QWidget *parent = nullptr);
    
signals:
    void machineStateClicked();
    void changeMachineStateClicked();
    void changeProductClicked();
    void changeUserClicked();
    void diagStateClicked();
    
public slots:
    void setMachineState(const PluginInterface::MachineState &machine_state, const PluginInterface::DiagState &diag_state);
    void setProduct(const QString &product);
    void setUsername(const QString &name);
    void setCanShowProductWindow(const bool canShowProductWindow);
    
private:
    QPushButton *_machine_state_button;
    StatusBarControlWidget *_state_control;
    StatusBarControlWidget *_product_control;
    StatusBarControlWidget *_user_control;
    QPushButton *_diag_state_button;
    
    QColor stateColor(const PluginInterface::MachineState &machine_state, const PluginInterface::DiagState &diag_state) const;
    QColor diagStateColor(const PluginInterface::DiagState &diag_state) const;
    QString stateName(const PluginInterface::MachineState &machine_state) const;
};

#endif // STATUSBARWIDGET_H
