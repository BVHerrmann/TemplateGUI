#ifndef ACCESSCONTROLOPTIONPANEL_H
#define ACCESSCONTROLOPTIONPANEL_H

#include <optionpanel.h>

#include <QtWidgets>

class AccessControlOptionPanel : public OptionPanel
{
    Q_OBJECT
public:
    explicit AccessControlOptionPanel(QWidget *parent = nullptr);
 
private:
	void setupUi();

signals: 
	void updatedUsers();

private slots:
	void importUsers();
	void exportUsers();
};

#endif // ACCESSCONTROLOPTIONPANEL_H
