#ifndef DEVICEPOPUPDIALOG_H
#define DEVICEPOPUPDIALOG_H

#include <popupdialog.h>

#include <QtWidgets>

#include "devicemanagement.h"

class DevicePopupDialog : public PopupDialog
{
    Q_OBJECT
  public:
    explicit DevicePopupDialog(QWidget* parent = nullptr);

    DEVICE device() const { return _device; }

  signals:

  public slots:

  protected:
    DEVICE _device;

  private:
    QList<QVariant> getDepartmentNumbers(const QString data);
};

#endif  // DEVICEPOPUPDIALOG_H
