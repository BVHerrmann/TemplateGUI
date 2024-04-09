#ifndef USERIMPORTPOPUPDIALOG_H
#define USERIMPORTPOPUPDIALOG_H

#include <popupdialog.h>

#include <QtWidgets>

#include "usermanagement.h"


class UserImportPopupDialog : public PopupDialog
{
    Q_OBJECT
public:
    explicit UserImportPopupDialog(QWidget *parent = nullptr);
    
	UserManagement::UserImportOption option() { return _option; }
signals:
    
public slots:
    
protected:

	UserManagement::UserImportOption _option = UserManagement::append;
};

#endif // USERIMPORTPOPUPDIALOG_H
