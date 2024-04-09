#ifndef PNIOTEXTWIDGET_H
#define PNIOTEXTWIDGET_H

#include "pniostatewidget.h"


class PNIOTextWidget : public PNIOStateWidget
{
public:
    PNIOTextWidget(const std::shared_ptr<PNIOValue> &config, QWidget *parent = nullptr);

public slots:

protected:
    QWidget * contentWidget() override;

    void setValue(const QVariant &value) override;

    void setState(bool valid) override;
    void setForced(bool forced) override;
    
    QLabel *_label;
};

#endif // PNIOTEXTWIDGET_H
