#ifndef PNIOSTATEWIDGET_H
#define PNIOSTATEWIDGET_H

#include <QtCore>
#include <QtWidgets>

#include <logic.h>
#include <pniovalue.h>


class PNIOStateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PNIOStateWidget(const std::shared_ptr<PNIOValue> &config, QWidget *parent = nullptr);

signals:
    // CommunicationInterface
    void valueChanged(const QString &name, const QVariant &value);

public slots:
    void updateUi();

protected:
    void setupUi();

    virtual QWidget * contentWidget() = 0;

    virtual void setValue(const QVariant &value) = 0;
    void changeValueTo(const QVariant &value);

    virtual void setState(bool valid) = 0;
    virtual void setForced(bool forced) = 0;
    
    bool _valid = false;
    bool _forced = false;
    QVariant _value;
    
    std::shared_ptr<PNIOValue> _config;
};

#endif // PNIOSTATEWIDGET_H
