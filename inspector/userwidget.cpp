#include "userwidget.h"

#include <QtGui>
#include <QtWidgets>

#include "bmessagebox.h"
#include "loginpopupdialog.h"
#include "usercardpopupdialog.h"



UserWidget::UserWidget(const USER user, QWidget* parent) : QWidget(parent)
{
	_user = user;
    setupUi();
}

void UserWidget::setupUi()
{
    QSettings settings;

    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

	layout->addWidget(new QLabel(_user.name));
	
	//access level
	QComboBox* accessLevel = new QComboBox();
	accessLevel->addItem(tr("User"), kAccessLevelUser);
	accessLevel->addItem(tr("Service"), kAccessLevelService);
	accessLevel->addItem(tr("SysOp"), kAccessLevelSysOp);
	accessLevel->addItem(tr("Admin"), kAccessLevelAdmin);

	int index = accessLevel->findData(_user.accessLevel);
	accessLevel->setCurrentIndex(index);

	connect(accessLevel, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { 
		UserManagement::instance().editUser(_user.uuid, UserManagement::accessLevel, accessLevel->itemData(index).toInt()); 
	});

	layout->addWidget(accessLevel);

	//password Button
	QPushButton *password = new QPushButton(tr("Change Password"));
	connect(password, &QPushButton::clicked, this, &UserWidget::setPassword);
	layout->addWidget(password);

	if (!settings.value(kDeviceComPort).toString().isEmpty()) {
		//cardId Button
		QPushButton* cardId = new QPushButton(tr("Add Transponder-ID"));
		connect(cardId, &QPushButton::clicked, this, &UserWidget::waitForCardId);
		layout->addWidget(cardId);
	}

	//delete Button
	QPushButton *deleteUser = new QPushButton(tr("Delete"));
	deleteUser->setObjectName("delete");
	connect(deleteUser, &QPushButton::clicked, [=]()
	{
		UserManagement::instance().deleteUser(_user.uuid);
		deleteLater();
	});
	layout->addWidget(deleteUser);
}

void UserWidget::setPassword()
{
	LoginPopupDialog m(window()->topLevelWidget());
	bool ok = m.exec();
	QString password = m.password();

	UserManagement* userManagement = &UserManagement::instance();

	if (ok) {
		if (password == QString() || userManagement->containsProperty(UserManagement::password, qHash(password))) {
			BMessageBox::warning(window()->topLevelWidget(), tr("Invalid Password"), tr("The password you have entered is invalid!"), QMessageBox::Ok);
		}
		else {
			userManagement->editUser(_user.uuid, UserManagement::password, qHash(password));
		}
	}
}

void UserWidget::waitForCardId()
{
	UserCardPopupDialog m(window()->topLevelWidget());
	_waitToReceiveCardId = true;
	emit waitToReceiveCardId(true);
	connect(this, &UserWidget::cardIdSet, [&]() { m.accept(); });
	bool ok = m.exec();

	if (!ok) {
		emit waitToReceiveCardId(false);
	}
}

void UserWidget::setUserCardId(const QString cardId) 
{
	UserManagement* userManagement = &UserManagement::instance();

	if (_waitToReceiveCardId) {
		if (userManagement->containsProperty(UserManagement::cardId, cardId)) {
			emit cardIdSet();
			BMessageBox::warning(window()->topLevelWidget(), tr("Invalid Transponder-ID"), tr("The Transponder-ID is already in use!"), QMessageBox::Ok);
		}
		else {
			userManagement->editUser(_user.uuid, UserManagement::cardId, cardId);
			_waitToReceiveCardId = false;
			emit cardIdSet();
		}
	}
}
