#ifndef ETRSCONFIGWIDGET_H
#define ETRSCONFIGWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "ETRSDeviceWidget.h"
#include "devicemanagement.h"
#include "etrsplugin.h"

class ETRSConfigWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ETRSConfigWidget(ETRSPlugin* plugin, QWidget* parent = 0);
    ~ETRSConfigWidget();

  public slots:
    void setDevice();

  private:
    void setupUi();
    void refreshDeviceListing(QWidget* widget);
    void deleteDevice(const DEVICE device, QWidget* deviceWidget);
    QList<QVariant> getDepartmentNumbers(const QString data);

    ETRSPlugin* _plugin;
};

#endif  // ETRSCONFIGWIDGET_H
