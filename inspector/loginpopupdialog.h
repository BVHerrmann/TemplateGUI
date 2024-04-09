#ifndef LOGINPOPUPDIALOG_H
#define LOGINPOPUPDIALOG_H

#include <popupdialog.h>

#include <QtWidgets>


class LoginPopupDialog : public PopupDialog
{
    Q_OBJECT
public:
    explicit LoginPopupDialog(QWidget *parent = nullptr);
    
    QString password() const { return _password; }
    
signals:
    
public slots:
    
protected:
    QString _password;
};

#endif // LOGINPOPUPDIALOG_H
