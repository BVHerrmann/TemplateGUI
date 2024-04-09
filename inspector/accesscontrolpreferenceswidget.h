#ifndef ACCESSCONTROLPREFERENCESWIDGET_H
#define ACCESSCONTROLPREFERENCESWIDGET_H

#include "preferenceswidget.h"

#include "usermanagement.h"

class AccessControlPreferencesWidget : public PreferencesWidget
{
    Q_OBJECT
public:
    explicit AccessControlPreferencesWidget(QWidget *parent = 0);
    
	bool cardIdReceived(const QString cardId);

signals:
	void setUserCardId(const QString cardId);
    
public slots:
	void refreshUserListing();
	void setWaitToRecevieCardId(bool status);

private:
	void setUser();
    void setupUi();

	QWidget* _user_area_widget;
	bool _waitToReceiveCardId = false;

};

#endif // ACCESSCONTROLPREFERENCESWIDGET_H
