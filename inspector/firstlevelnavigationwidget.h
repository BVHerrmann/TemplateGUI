#ifndef FIRSTLEVELNAVIGATIONWIDGET_H
#define FIRSTLEVELNAVIGATIONWIDGET_H

class QPushButton;

#include "backgroundwidget.h"
#include "interfaces.h"


class FirstLevelNavigationWidget : public BackgroundWidget
{
    Q_OBJECT
public:
    explicit FirstLevelNavigationWidget(QWidget *parent = nullptr);

    void hideButton(const MainWindowInterface::WidgetType &type);
    void showButton(const MainWindowInterface::WidgetType &type);
    
signals:
    void buttonClicked(const MainWindowInterface::WidgetType &type);
    void takeScreenshot();
    void quit();
    
public slots:
    
private:
    QHash<MainWindowInterface::WidgetType, QPushButton *> _buttons;
};

#endif // FIRSTLEVELNAVIGATIONWIDGET_H
