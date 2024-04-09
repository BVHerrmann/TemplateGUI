#ifndef PNIOWIDGET_H
#define PNIOWIDGET_H

#include <QtCore>
#include <QtWidgets>

#include <pniobase.h>


#include "pniointerface.h"
class PNIOPlugin;
class PNIOStateWidget;
class PNIORecordWidget;


class PNIOWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PNIOWidget(PNIOPlugin *plugin, QWidget *parent = nullptr);
    ~PNIOWidget();

signals:

public slots:
    void setupWidget(PNIOInterface *pnio_interface);

private:
    void setupUi();

    PNIOStateWidget* newWidgetForConfig(const std::shared_ptr<PNIOValue> &config);
	PNIORecordWidget* newWidgetForRecordConfig(const std::shared_ptr<PNIORecord> &config);

	void addLine(QFormLayout* layout);

    QFormLayout *_in_layout;
    QFormLayout *_out_layout;

    PNIOPlugin *_plugin;

    QTimer *_update_timer;
};

#endif // PNIOWIDGET_H