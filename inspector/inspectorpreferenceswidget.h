#ifndef INSPECTORPREFERENCESWIDGET_H
#define INSPECTORPREFERENCESWIDGET_H

#include "preferenceswidget.h"

#include "pluginlistmodel.h"


class InspectorPreferencesWidget : public PreferencesWidget
{
    Q_OBJECT
public:
    explicit InspectorPreferencesWidget(PluginsController *pluginsController, QWidget *parent = 0);
    
signals:
    
public slots:
    void changeLanguage(int index);

private:
    void setupUi();
    
    PluginListModel *_plugins_model;
};

#endif // INSPECTORPREFERENCESWIDGET_H
