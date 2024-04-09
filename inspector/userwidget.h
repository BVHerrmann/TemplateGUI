#ifndef USERWIDGET_H
#define USERWIDGET_H

#include <QWidget>

#include "interfaces.h"
#include "inspectordefines.h"

#include "usermanagement.h"

class UserWidget : public QWidget
{
    Q_OBJECT

public:
	explicit UserWidget(const USER user, QWidget *parent = 0);

signals:
	void waitToReceiveCardId(bool status);
	void cardIdSet();

public slots:
	void setUserCardId(const QString cardId);

private slots:
	void setPassword();
	void waitForCardId();

private:
    void setupUi();

	USER _user;
	bool _waitToReceiveCardId = false;
};

#endif // USERWIDGET_H
