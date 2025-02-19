#include "pniostatewidget.h"


PNIOStateWidget::PNIOStateWidget(const std::shared_ptr<PNIOValue> &config, QWidget *parent) :
    QWidget(parent)
{
    _config = config;
}

void PNIOStateWidget::setupUi()
{
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(layout->margin() / 4);

    layout->addStretch();

    QWidget *widget = contentWidget();
    layout->addWidget(widget);

	setState(false);
}

void PNIOStateWidget::changeValueTo(const QVariant &value)
{
    _config->force(value);
}

void PNIOStateWidget::updateUi()
{
    bool changed = false;
    
    bool valid = _config->valid();
    if (valid != _valid) {
        setState(valid);
        _valid = valid;
        changed = true;
    }
    
    bool forced = _config->is_forced();
    if (forced != _forced) {
        setForced(forced);
        _forced = forced;
        changed = true;
    }
    
    QVariant value = _config->displayValue();
    if (value != _value) {
        setValue(value);
        _value = value;
        changed = true;
    }

    if (changed)
        QWidget::update();
}
