#ifndef USERPOPUPDIALOG_H
#define USERPOPUPDIALOG_H

#include <popupdialog.h>

#include <QtWidgets>

#include "usermanagement.h"


class UserPopupDialog : public PopupDialog
{
    Q_OBJECT
public:
    explicit UserPopupDialog(QWidget *parent = nullptr);
    
    USER user() const { return _user; }
    
signals:
    
public slots:
    
protected:
	USER _user;

};

#endif // USERPOPUPDIALOG_H
