#include "userpopupdialog.h"

#include <QtWidgets>
#include <QSerialPortInfo>

#include "interfaces.h"
#include "bmessagebox.h"


UserPopupDialog::UserPopupDialog(QWidget *parent) : PopupDialog(parent)
{
	_user.uuid = QUuid::createUuid().toString();

	setWindowTitle(tr("Add User"));
	
	QGridLayout* grid = new QGridLayout();
	centralWidget()->setLayout(grid);
	
	grid->setRowMinimumHeight(0, 16);

	//Name
	grid->addWidget(new QLabel(tr("Name:")), 1, 0);
	QLineEdit* nameInput = new QLineEdit();
	connect(nameInput, &QLineEdit::textChanged, [=](const QString &value) { _user.name = value; });
	grid->addWidget(nameInput, 1, 1);

	//Access level
	grid->addWidget(new QLabel(tr("Access level:")), 2, 0);
	QComboBox* accessLevel = new QComboBox();

	accessLevel->addItem(tr("User"), kAccessLevelUser);
	accessLevel->addItem(tr("Service"), kAccessLevelService);
	accessLevel->addItem(tr("SysOp"), kAccessLevelSysOp);
	accessLevel->addItem(tr("Admin"), kAccessLevelAdmin);

	int index = accessLevel->findData(kAccessLevelUser);
	accessLevel->setCurrentIndex(index);
	_user.accessLevel = accessLevel->itemData(index).toInt();

	connect(accessLevel, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { _user.accessLevel = accessLevel->itemData(index).toInt(); });
	grid->addWidget(accessLevel, 2, 1);

	//password
	grid->addWidget(new QLabel(tr("Password:")), 3, 0);
	QLineEdit* passwordInput = new QLineEdit();
	passwordInput->setEchoMode(QLineEdit::Password);
	connect(passwordInput, &QLineEdit::textChanged, [=](const QString &value) { _user.passwordHash = qHash(value); });
	grid->addWidget(passwordInput, 3, 1);

	grid->setRowMinimumHeight(4, 16);

	//Buttons
	QDialogButtonBox* buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		Qt::Horizontal);

	grid->addWidget(buttonBox, 5, 1);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
		if (_user.passwordHash == qHash(QString()) || UserManagement::instance().containsProperty(UserManagement::password, _user.passwordHash)) {
			BMessageBox::warning(this, tr("Invalid Password"), tr("The password you have entered is invalid!"), QMessageBox::Ok);
		}
		else {
			this->accept();
		}
	});

}