#include "tabwidget.h"

#include <QtWidgets>

#include "mainwidget.h"
#include "pluginscontroller.h"


TabWidget::TabWidget(QWidget *parent) : BackgroundWidget(parent)
{
    setLayout(new QStackedLayout());
    layout()->setContentsMargins(0, 0, 0, 0);
    
    _pane_widget = new BackgroundWidget();
    _pane_widget->setObjectName("pane");
    _pane_widget->setLayout(new QHBoxLayout());
    _pane_widget->layout()->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(_pane_widget);
    
    _tab_widget = new SecondLevelNavigationWidget(this);
    layout()->addWidget(_tab_widget);
    
    _current_plugin = nullptr;
}

int TabWidget::count() const
{
    return _tab_widget->count() ? _tab_widget->count() : _current_plugin != nullptr;
}

void TabWidget::registerModuleWidget(QWidget *widget, const QString &label, const int tabIndex)
{
    // store preferred index in widget
    widget->setProperty("preferredTabIndex", tabIndex);
    
    // currently only showing main widget. add this one first
    if (_current_plugin && _current_plugin != widget) {
        _tab_widget->addTab(_current_plugin, _current_title);
        _current_plugin = nullptr;
        qobject_cast<QStackedLayout *>(layout())->setCurrentWidget(_tab_widget);
    }
    
    if (_current_plugin != widget) {
        // widget not already in tab
        if (_tab_widget->indexOf(widget) == -1) {
            // find correct position to insert
            int insertPos = 0;
            while(insertPos < _tab_widget->count()) {
                QWidget *checkWidget = _tab_widget->widget(insertPos);
                if (checkWidget->dynamicPropertyNames().contains("preferredTabIndex") && checkWidget->property("preferredTabIndex").toInt() <= tabIndex) {
                    insertPos++;
                } else {
                    break;
                }
            }
            
            // insert
            _tab_widget->insertTab(insertPos, widget, label);
            
            // select first tab
            _tab_widget->setCurrentIndex(0);
        }
        
        // show single widget
        if (_tab_widget->count() == 1) {
            makeWidgetMain(_tab_widget->widget(0));
        }
    }
}

MainWidget * TabWidget::mainWidget(MainWindowInterface *window_interface, int idx)
{
    auto pair = std::make_pair(window_interface, idx);
    
    MainWidget *widget = _main_widgets[pair];
    if (!widget) {
        widget = new MainWidget(window_interface->mainWidget(idx), window_interface->optionPanel(idx), this);
        _main_widgets[pair] = widget;
    } else {
        widget->update(window_interface->mainWidget(idx), window_interface->optionPanel(idx));
    }
    
    return widget;
}

void TabWidget::updateUi(PluginsController* pluginsController, const MainWindowInterface::WidgetType& type, int current_access_level, const QString& username, const QString& uuid,
                         const QString& card_id)
{
    // show / remove tabs
    for (const QObject *plugin : pluginsController->loadedPlugins()) {
        MainWindowInterface *iMainWindow = qobject_cast<MainWindowInterface *>(plugin);
        if (iMainWindow) {
            for (int idx=0; idx < iMainWindow->mainWidgetCount(); idx++) {
                if (iMainWindow->widgetType(idx) == type) {
                    QWidget *main_widget = mainWidget(iMainWindow, idx);
                    if (current_access_level >= iMainWindow->requiredWidgetAccessLevel(idx)) {
                        iMainWindow->setAccessLevel(main_widget, current_access_level, username, uuid, card_id);
                        registerModuleWidget(main_widget, iMainWindow->title(idx), iMainWindow->preferredTabIndex(idx));
                    } else {
                        removeTab(main_widget);
                    }
                }
            }
        }
    }
    
    // remove tabs that should not be visible anymore
    for (int i = _tab_widget->count() - 1; i >= 0; i--) {
        QWidget *widget = _tab_widget->widget(i);
        bool found = false;
        
        // check if any of the plugins owns this widget
        for (const QObject *plugin : pluginsController->loadedPlugins()) {
            MainWindowInterface *iMainWindow = qobject_cast<MainWindowInterface *>(plugin);
            if (iMainWindow) {
                for (int idx=0; idx < iMainWindow->mainWidgetCount(); idx++) {
                    if (iMainWindow->widgetType(idx) == type && mainWidget(iMainWindow, idx) == widget) {
                        found = true;
                        break;
                    }
                }
            }
            if (found) {
                break;
            }
        }
        if (!found) {
            removeTab(widget);
        }
    }
}

void TabWidget::setCurrentIndex(int index)
{
    _tab_widget->setCurrentIndex(index);
}

void TabWidget::removeTab(QWidget *widget)
{
    int i = _tab_widget->indexOf(widget);
    if (i != -1) {
        _tab_widget->removeTab(i);

        // remove tab widget in case there is ony one widget
        if (_tab_widget->count() == 1) {
            makeWidgetMain(_tab_widget->widget(0));
        }
    } else if (_current_plugin == widget) {
        // this was the last widget, so reset
        _current_plugin = nullptr;
        _current_title = QString();
    }
}

void TabWidget::makeWidgetMain(QWidget *widget)
{
    // save current plugin
    _current_plugin = widget;
    _current_title = _tab_widget->tabText(0);
    
    // show pane widget only
    _pane_widget->layout()->addWidget(widget);
    qobject_cast<QStackedLayout *>(layout())->setCurrentWidget(_pane_widget);
    widget->setHidden(false);
}
