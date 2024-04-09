#ifndef PNIOBITSTATEWIDGET_H
#define PNIOBITSTATEWIDGET_H

#include "pniostatewidget.h"

#include <bswitch.h>


class PNIOBitStateWidget : public PNIOStateWidget
{
    Q_OBJECT
public:
    explicit PNIOBitStateWidget(const std::shared_ptr<PNIOValue> &config, QWidget *parent = nullptr);

signals:

public slots:
    void setOutputState();

protected:
    QWidget * contentWidget() override;

    void setValue(const QVariant &value) override;

    void setState(bool valid) override;
    void setForced(bool forced) override;

    BSwitch *_state_button;
};

#endif // PNIOBITSTATEWIDGET_H
