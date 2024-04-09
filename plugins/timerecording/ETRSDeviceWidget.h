#ifndef ETRSDEVICEWIDGET_H
#define ETRSDEVICEWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "etrsplugin.h"

class ETRSDeviceWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ETRSDeviceWidget(QWidget* parent = 0);
    explicit ETRSDeviceWidget(const DEVICE device, QWidget* parent = 0);
    ~ETRSDeviceWidget();

  signals:
    void deleteWidget(const DEVICE device, QWidget* widget);

  private:
    void setupUi();
    DEVICE _device;
};

#endif  // ETRSDEVICEWIDGET_H
