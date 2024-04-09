#include "loginpopupdialog.h"

#include <QtWidgets>


LoginPopupDialog::LoginPopupDialog(QWidget *parent) : PopupDialog(parent)
{
    setWindowTitle(tr("Login"));
    
    QBoxLayout *box = new QVBoxLayout();
    centralWidget()->setLayout(box);
    
    box->addSpacing(16);
    
    QLabel *passwordLabel = new QLabel(tr("Password:"));
    box->addWidget(passwordLabel);
    
    QLineEdit *passwordLineEdit = new QLineEdit();
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    connect(passwordLineEdit, &QLineEdit::returnPressed, this, &LoginPopupDialog::accept);
    connect(passwordLineEdit, &QLineEdit::textChanged, [=](const QString &value) { _password = value; });
    box->addWidget(passwordLineEdit);
    
    box->addSpacing(16);
    
    // buttons
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    button_box->button(QDialogButtonBox::Cancel)->setObjectName("close");
    button_box->button(QDialogButtonBox::Ok)->setObjectName("action");
    button_box->button(QDialogButtonBox::Ok)->setText(tr("Login"));
    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    box->addWidget(button_box);
    
    // focus on password field
    passwordLineEdit->setFocus();
}
