#include "pniotextwidget.h"

#include <colors.h>


PNIOTextWidget::PNIOTextWidget(const std::shared_ptr<PNIOValue> &config, QWidget *parent) :
    PNIOStateWidget(config, parent)
{
    _label = new QLabel();

    setupUi();
}

QWidget * PNIOTextWidget::contentWidget()
{
    return _label;
}

void PNIOTextWidget::setValue(const QVariant &value)
{
    _label->setText(value.toString());
}

void PNIOTextWidget::setState(bool valid)
{
    _label->setEnabled(valid);
}

void PNIOTextWidget::setForced(bool forced)
{
    QPalette pal(_label->palette());

    if (forced) {
        pal.setColor(QPalette::Text, HMIColor::WarningHigh);
    } else {
        pal.setColor(QPalette::Text, HMIColor::DarkGrey);
    }

    _label->setPalette(pal);
}
