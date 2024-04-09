#include "accesscontrolpreferenceswidget.h"

#include <QtGui>
#include <QtWidgets>
#include <QSerialPortInfo>

#include <audittrail.h>
#include <bdistancespinbox.h>

#include "interfaces.h"
#include "inspectordefines.h"
#include "loginpopupdialog.h"

#include "userwidget.h"
#include "userimportpopupdialog.h"
#include "userpopupdialog.h"

#include <algorithm>
#include <functional>
#include <array>
#include <iostream>

AccessControlPreferencesWidget::AccessControlPreferencesWidget(QWidget *parent) :
    PreferencesWidget(parent)
{
    setupUi();
}

void AccessControlPreferencesWidget::setUser()
{
	UserPopupDialog m(this);
	bool ok = m.exec();
	USER user = m.user();

	if (ok) {
		UserManagement::instance().addUser(user);
	}
}

void AccessControlPreferencesWidget::setupUi()
{
	QSettings settings;

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);

	//common
	QGroupBox *common_group = new QGroupBox(tr("Common"));
	QFormLayout *common_layout = new QFormLayout();
	common_group->setLayout(common_layout);
	layout->addWidget(common_group);

	BDistanceSpinBox *timeout_spinbox = new BDistanceSpinBox();
    timeout_spinbox->setProperty(kAuditTrail, tr("Automatically logout after"));
	timeout_spinbox->setProperty(kPropertyKey, kLogoutTimeout);
	timeout_spinbox->setRange(0, 60);
    timeout_spinbox->setDecimals(0);
    timeout_spinbox->setSuffix(" min");
	timeout_spinbox->setFactor(60 * 1000); // ms to min
	timeout_spinbox->setStoredValue(settings.value(kLogoutTimeout, kDefaultLogoutTimeout).toDouble());
	connect(timeout_spinbox, static_cast<void (BDistanceSpinBox::*)(double)>(&BDistanceSpinBox::storedValueChanged), this, static_cast<void (AccessControlPreferencesWidget::*)(int)>(&AccessControlPreferencesWidget::changeValue));

	common_layout->addRow(tr("Automatically logout after"), timeout_spinbox);

	//COM-Port
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

	QComboBox* comPort = new QComboBox();
    comPort->setProperty(kAuditTrail, tr("RFID Reader"));
	comPort->addItem(tr("No COM-Port selected"), "No COM-Port selected");

	for (int i = 0; i < ports.size(); i++) {
		QString port = QString(ports.at(i).portName() + " (" + ports.at(i).description() + ")");
		comPort->addItem(port, ports.at(i).portName());
	}

	QString currentText = settings.value(kDeviceComPort).toString();

	if (!currentText.isEmpty()) {
		int index = comPort->findData(currentText);
		if (index == -1) {
			comPort->addItem(currentText, currentText);
			comPort->setCurrentText(currentText);
		}
		else {
		comPort->setCurrentIndex(index);
		}
	}

	connect(comPort, QOverload<int>::of(&QComboBox::currentIndexChanged),
		[=](int index) {
		QSettings settings;
		if (comPort->itemData(index) == "No COM-Port selected") {
			settings.remove(kDeviceComPort);
		}
		else {
			settings.setValue(kDeviceComPort, comPort->currentData());
		}
	});
	common_layout->addRow(tr("RFID Reader"), comPort);

	//Users
	QGroupBox *user_group = new QGroupBox(tr("Users"));
	QVBoxLayout *user_layout = new QVBoxLayout();
	user_group->setLayout(user_layout);
	layout->addWidget(user_group);

	QScrollArea *user_area = new QScrollArea();
	user_area->setWidgetResizable(true);
	user_area->setFrameStyle(QFrame::Plain);
	user_layout->addWidget(user_area);

	_user_area_widget = new QWidget(user_area);
	user_area->setWidget(_user_area_widget);

	refreshUserListing();

	//user buttons
	QHBoxLayout* user_button_layout = new QHBoxLayout();

	user_button_layout->addStretch();
	QPushButton *addUser = new QPushButton(tr("Add User"));
	connect(addUser, &QPushButton::clicked, [=]()
	{
		setUser();
		refreshUserListing();
	});
	user_button_layout->addWidget(addUser);
	user_layout->addLayout(user_button_layout, 1);
}

void AccessControlPreferencesWidget::refreshUserListing()
{
	QWidget* widget = _user_area_widget;

	for (auto c : widget->children()) {
		c->deleteLater();
	}
	delete widget->layout();

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	widget->setLayout(layout);

	QList<USER> users = UserManagement::instance().users();
	std::sort(users.begin(), users.end(), [](const USER& a, const USER& b)->bool {return a.name < b.name; });

	for (int i = 0; i < users.size(); i++) {
		UserWidget* userWidget = new UserWidget(users.at(i), widget);
		connect(userWidget, &UserWidget::waitToReceiveCardId, this, &AccessControlPreferencesWidget::setWaitToRecevieCardId);
		connect(this, &AccessControlPreferencesWidget::setUserCardId, userWidget, &UserWidget::setUserCardId);
		layout->addWidget(userWidget);
	}

	layout->addStretch();
}

bool AccessControlPreferencesWidget::cardIdReceived(const QString cardId)
{
	if (_waitToReceiveCardId) {
		_waitToReceiveCardId = false;
		emit setUserCardId(cardId);
		return true;
	}

	return false;
}

void AccessControlPreferencesWidget::setWaitToRecevieCardId(bool status)
{
	_waitToReceiveCardId = status;
}
