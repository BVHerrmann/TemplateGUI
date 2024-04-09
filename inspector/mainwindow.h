#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class QStackedWidget;

#include "interfaces.h"
#include "tabwidget.h"
#include "accesscontrolpreferenceswidget.h"
#include "accesscontroloptionpanel.h"
#include "BSimaticident.h"
class FirstLevelNavigationWidget;
class PluginsController;
class StatusBarWidget;


class MainWindow : public QMainWindow, PluginInterface, MainWindowInterface, CommunicationInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface)
    Q_INTERFACES(MainWindowInterface)
    Q_INTERFACES(CommunicationInterface)
    
    Q_PROPERTY(int accessLevel READ currentAccessLevel)

public:
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();
    
    // PluginInterface
    const QString identifier() const override { return "Inspector"; }
    const QString name() const override { return identifier(); }
    const MachineState machineState() const override { return _current_state; }
    void updateMessages() override;
    
    // MainWindowInterface
    const WidgetType widgetType(const int idx) const override;
    const QString title(const int idx) const override;
    QWidget * mainWidget(const int idx) const override;
	OptionPanel *optionPanel(const int idx) const override;
    int preferredTabIndex(const int idx) const override;
    int requiredWidgetAccessLevel(const int idx) const override;
    int mainWidgetCount() const override { return 4; }
    
signals:
    // PluginInterface
    void localeChanged(const QString &locale);

    // CommunicationIntreface
    void valueChanged(const QString &name, const QVariant &value) override;
    void valuesChanged(const QHash<QString, QVariant> &values) override;

	//Signals for CardReader
	void accessStatus(const char color, unsigned int soundDurationInMS);

public slots:
    // PluginInterface
    void initialize() override { setAccessLevel(currentAccessLevel()); updateUi(); }
    void requestMachineState(const PluginInterface::MachineState state) override { _current_state = state; updateUi(); }
    
    // CommunicationInterface
    void setValue(const QString &name, const QVariant &value) override;
    void setValues(const QHash<QString, QVariant> &values) override;
    void setValue(const QObject * sender, const QString &name, const QVariant &value) { if (sender != this) setValue(name, value); }
    void setValues(const QObject * sender, const QHash<QString, QVariant> &values) { if (sender != this) setValues(values); }

    // PreferencesInterface
    void loadPreferences();
    void storePreferences();

    // QtSingleApplication
    void handleMessage(const QString &message);

    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
    void showMenu();
    
    void changeUser();
	void changeUser(const QString cardId);
    void setAccessLevel(int level = 0, QString username = QString(), QString uuid = QString(), QString card_id = QString());

    void takeScreenshot();
    
    void setPluginsController(PluginsController *pluginsController);

    void changeToFirstApplicationWidget();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    
private:
    void setupUi();
    void updateUi();
    void setupSystemTrayIcon();
    void terminateRequest();
    PluginInterface::MachineState _current_state = Production;
    
    QStackedWidget *_stack_widget;
    StatusBarWidget *_status_bar;
    FirstLevelNavigationWidget *_first_level_navigation;
    QHash<MainWindowInterface::WidgetType, TabWidget *> _section_widgets;
    QWidget *_overlay;
    QWidget *_menu;
    
    PluginsController *_pluginsController;
    QWidget *_machine_state_widget;
    QWidget *_alarm_widget;
    QWidget *_preferences_widget;
	AccessControlPreferencesWidget* _access_control_preferences;
	AccessControlOptionPanel* _access_control_option_panel;
    
    QTimer *_logout_timer;
    int _current_access_level;

    QSystemTrayIcon* _trayIcon = nullptr;
    
    bool _check_priority;
};

#endif // MAINWINDOW_H
