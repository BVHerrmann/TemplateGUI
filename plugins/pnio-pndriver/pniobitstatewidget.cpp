#include "pniobitstatewidget.h"

#include <colors.h>


PNIOBitStateWidget::PNIOBitStateWidget(const std::shared_ptr<PNIOValue> &config, QWidget *parent) :
    PNIOStateWidget(config, parent)
{
    _state_button = new BSwitch(this);
    _state_button->setCheckable(true);
    connect(_state_button, &BSwitch::clicked, this, &PNIOBitStateWidget::setOutputState);

    setupUi();
}

QWidget * PNIOBitStateWidget::contentWidget()
{
    return _state_button;
}

void PNIOBitStateWidget::setOutputState()
{
    changeValueTo(_state_button->isChecked());
}

void PNIOBitStateWidget::setValue(const QVariant &value)
{
    if (value.toBool())
        _state_button->setChecked(true);
    else
        _state_button->setChecked(false);
}

void PNIOBitStateWidget::setState(bool valid)
{
    _state_button->setEnabled(valid);
}

void PNIOBitStateWidget::setForced(bool forced)
{
    _state_button->setCheckedColor(forced ? HMIColor::WarningHigh : HMIColor::OK);
    _state_button->setUncheckedColor(forced ? HMIColor::WarningLow : HMIColor::Light);
}
