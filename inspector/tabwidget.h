#ifndef TABWIDGET_H
#define TABWIDGET_H

class QTabWidget;

#include <backgroundwidget.h>
#include <interfaces.h>
#include <secondlevelnavigationwidget.h>

#include "mainwidget.h"

class PluginsController;


class TabWidget : public BackgroundWidget
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget *parent = nullptr);
    
    int count() const;
    
    void updateUi(PluginsController* pluginsController, const MainWindowInterface::WidgetType& type, int current_access_level, const QString& username, const QString& uuid, const QString& card_id);

    void setCurrentIndex(int index);
    
signals:
    
public slots:
    
private:
    SecondLevelNavigationWidget *_tab_widget;
    QWidget *_pane_widget;
    
    QWidget *_current_plugin;
    QString _current_title;
    
    void removeTab(QWidget *widget);
    void makeWidgetMain(QWidget *widget);
    
    void registerModuleWidget(QWidget *widget, const QString &label, const int tabIndex);
    MainWidget * mainWidget(MainWindowInterface *window_interface, int idx);
    std::map<std::pair<MainWindowInterface *, int>, MainWidget *> _main_widgets;
};

#endif // TABWIDGET_H
