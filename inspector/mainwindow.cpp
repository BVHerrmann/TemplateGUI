#include "mainwindow.h"

#include <QtGui>
#include <QtWidgets>

#include <audittrail.h>
#include <bmessagebox.h>

#include "alarmwidget.h"
#include "inspectordefines.h"
#include "inspectorpreferenceswidget.h"
#include "loginpopupdialog.h"
#include "machinestatepopupdialog.h"
#include "messagespopupdialog.h"
#include "pluginscontroller.h"
#include "titlebarwidget.h"
#include "statusbarwidget.h"
#include "firstlevelnavigationwidget.h"
#include "machinestatewidget.h"

#include "usermanagement.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    _pluginsController = nullptr;
    
    // set lowest access level as default
    _current_access_level = kAccessLevelGuest;
    
    // prepare ui
    setupUi();
    
    // setting
    _alarm_widget = nullptr;
    _preferences_widget = nullptr;
    _machine_state_widget = nullptr;
    _access_control_preferences = new AccessControlPreferencesWidget();
	_access_control_option_panel = new AccessControlOptionPanel();
	connect(_access_control_option_panel, &AccessControlOptionPanel::updatedUsers, _access_control_preferences, &AccessControlPreferencesWidget::refreshUserListing);
    
    // logout timer
    _logout_timer = new QTimer(this);
    _logout_timer->setInterval(kDefaultLogoutTimeout);
    _logout_timer->setSingleShot(true);
    connect(_logout_timer, &QTimer::timeout, [=]() { _status_bar->setUsername(QString()); this->setAccessLevel(); });

    // restore settings
    loadPreferences();
}

MainWindow::~MainWindow()
{
    _preferences_widget->deleteLater();
    _access_control_preferences->deleteLater();
}

void MainWindow::setupUi()
{
    setWindowTitle(QCoreApplication::applicationName());
    setCentralWidget(new QWidget());
    
    // stacked layout
    QStackedLayout *_stacked_layout = new QStackedLayout(centralWidget());
    _stacked_layout->setStackingMode(QStackedLayout::StackAll);
    
    // main widget
    QWidget *main_widget = new QWidget();
    _stacked_layout->addWidget(main_widget);
    
    QVBoxLayout *main_layout = new QVBoxLayout(main_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    
    TitleBarWidget *title_bar = new TitleBarWidget();
    connect(title_bar, &TitleBarWidget::titleClicked, [=]() { changeToFirstApplicationWidget(); });
    connect(title_bar, &TitleBarWidget::openFirstLevelNavigationClicked, this, &MainWindow::showMenu);
    main_layout->addWidget(title_bar);
    
    _status_bar = new StatusBarWidget(this);
    connect(_status_bar, &StatusBarWidget::machineStateClicked, [=]() { if (_pluginsController) { MessagesPopupDialog m(_pluginsController, this); m.exec(); } });
    connect(_status_bar, &StatusBarWidget::changeMachineStateClicked, [=]() { if (_pluginsController) { MachineStatePopupDialog m(_pluginsController, this); m.exec(); }  });
    connect(_status_bar, &StatusBarWidget::changeProductClicked, [=]() { if(_pluginsController) { _pluginsController->showProductWindow(); } });
    connect(_status_bar, &StatusBarWidget::changeUserClicked, this, static_cast<void (MainWindow::*)()>(&MainWindow::changeUser));
    connect(_status_bar, &StatusBarWidget::diagStateClicked, [=]() { if(_pluginsController) { _pluginsController->resetPlugins(); } });
    main_layout->addWidget(_status_bar);
    
    // center
    _stack_widget = new QStackedWidget();
    _section_widgets[MainWindowInterface::Application] = new TabWidget();
    _stack_widget->addWidget(_section_widgets[MainWindowInterface::Application]);
    _section_widgets[MainWindowInterface::Statistics] = new TabWidget();
    _stack_widget->addWidget(_section_widgets[MainWindowInterface::Statistics]);
    _section_widgets[MainWindowInterface::Messages] = new TabWidget();
    _stack_widget->addWidget(_section_widgets[MainWindowInterface::Messages]);
    _section_widgets[MainWindowInterface::Diagnostics] = new TabWidget();
    _stack_widget->addWidget(_section_widgets[MainWindowInterface::Diagnostics]);
    _section_widgets[MainWindowInterface::Settings] = new TabWidget();
    _stack_widget->addWidget(_section_widgets[MainWindowInterface::Settings]);
    main_layout->addWidget(_stack_widget, 1);
    
    // Overlay
    _overlay = new QWidget();
    _overlay->setObjectName("overlay");
    _stacked_layout->addWidget(_overlay);
    _stacked_layout->setCurrentWidget(_overlay);
    _overlay->hide();
    
    // First Level Navigation
    _menu = new QWidget();
    QHBoxLayout *menu_layout = new QHBoxLayout();
    menu_layout->setContentsMargins(0, 0, 0, 0);
    menu_layout->setSpacing(0);
    
    QPushButton *close_menu_button = new QPushButton();
    close_menu_button->setObjectName("invisible");
    connect(close_menu_button, &QPushButton::clicked, _menu, &QWidget::hide);
    menu_layout->addWidget(close_menu_button);
    
    _first_level_navigation = new FirstLevelNavigationWidget();
    connect(_first_level_navigation, &FirstLevelNavigationWidget::buttonClicked, [=](const MainWindowInterface::WidgetType &type) { _menu->hide(); _stack_widget->setCurrentWidget(_section_widgets[type]); });
    connect(_first_level_navigation, &FirstLevelNavigationWidget::takeScreenshot, this, &MainWindow::takeScreenshot);
    connect(_first_level_navigation, &FirstLevelNavigationWidget::quit, [=]() { terminateRequest(); });
    menu_layout->addWidget(_first_level_navigation);

    _menu->setLayout(menu_layout);
    _stacked_layout->addWidget(_menu);
    _stacked_layout->setCurrentWidget(_menu);
    _menu->hide();

    if (QSettings().value(kSystemTrayIcon, kDefaultSystemTrayIcon).toBool()) {
        setupSystemTrayIcon();
    }
    
    updateUi();
}

void MainWindow::updateMessages()
{
#ifdef Q_OS_WIN
    if (_check_priority) {
        DWORD dwPriClass = GetPriorityClass(GetCurrentProcess());
        if (dwPriClass != REALTIME_PRIORITY_CLASS) {
            QList<PluginInterface::Message> messages;
            messages << PluginInterface::Message(256, tr("Not running at highest Priority!"), QtWarningMsg);
            PluginInterface::updateMessages(messages);
        }
    }
#endif
}

const MainWindow::WidgetType MainWindow::widgetType(const int idx) const
{
    switch (idx) {
        case 1:
            return Messages;
            break;
        case 3: 
            return Application;
            break;
        default:
            return Settings;
    }
}

const QString MainWindow::title(const int idx) const
{
    switch(idx) {
        case 1:
            return tr("Alarm History");
            break;
        case 2:
            return tr("User Management");
            break;
        case 3: 
            return tr("Messages");
            break;
        default:
            return tr("Application");
    }
}

QWidget * MainWindow::mainWidget(const int idx) const
{
    switch(idx) {
        case 0:
            return _preferences_widget;
            break;
        case 1:
            return _alarm_widget;
            break;
        case 2:
            return _access_control_preferences;
            break;
        case 3:
            return _machine_state_widget;
            break;
        default:
            return nullptr;
    }
}
OptionPanel * MainWindow::optionPanel(const int idx) const
{
	switch (idx) {
	    case 2: {	
		    return currentAccessLevel() >= kAccessLevelAdmin ? _access_control_option_panel : nullptr;
		    break;
	    }
	    default:
		    return nullptr;
	}
}

int MainWindow::preferredTabIndex(const int idx) const
{
    switch (idx) {
        case 1:
            return 100;
            break;
        case 3:
            return 0;
            break;
        default:
            return INT_MAX - mainWidgetCount() + idx;
    }
}

int MainWindow::requiredWidgetAccessLevel(const int idx) const
{
    switch(idx) {
        case 1:
            return kAccessLevelUser;
            break;
        case 2:
            return kAccessLevelAdmin;
            break;
        case 3:
            return _pluginsController && _pluginsController->machineState() <= Initializing ? kAccessLevelGuest : kAccessLevelNA;
            break;
        default:
            return kAccessLevelBertram;
    }
}

void MainWindow::setValue(const QString &name, const QVariant &value)
{
    if (name == "updateUi") {
        updateUi();
    } else if (name == "AccessLevel" && _current_access_level != value.toInt()) {
        setAccessLevel(value.toInt());
    } else if (name == "updateUsers") {
        auto* user_management = &UserManagement::instance();

        auto user_array = value.toJsonArray();
        for (auto element : user_array) {
            auto user_object = element.toObject();

            USER user;
            user.uuid = user_object.value("uuid").toString();
            user.name = user_object.value("name").toString();
            user.accessLevel = user_object.value("access_level").toInt();
            user.passwordHash = qHash(user_object.value("password").toString());
            user.cardId = user_object.value("card_id").toString();

            user_management->addUser(user);
        }
    }
}

void MainWindow::setValues(const QHash<QString, QVariant> &values)
{
    defaultSetValues(values);
}

void MainWindow::handleMessage(const QString& message)
{
    if (message == "hide") {
#ifdef Q_OS_WIN
        hide();
#else
        showMinimized();
#endif
    } else {
        show();
        activateWindow();
    }
}

bool MainWindow::event(QEvent *event)
{
    switch(event->type()) {
        case QEvent::HoverMove:
            if (_current_access_level > UserManagement::instance().getBaseAccessLevel() && _logout_timer->interval()) {
                _logout_timer->start();
            }
            break;
        
        // show/hide overlay
        case QEvent::WindowBlocked:
            _overlay->show();
            break;
        case QEvent::WindowUnblocked:
            _overlay->hide();
            break;
            
        default:
            break;
    }

    return QMainWindow::event(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    storePreferences();
    if (_trayIcon) {
    #ifdef Q_OS_MACOS
        if (!event->spontaneous() || !isVisible()) {
            return;
        }
    #endif
        if (_trayIcon->isVisible()) {
            hide();
        } else {
            terminateRequest();
        }
    }
    event->ignore();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    // event filter is only active in full screen mode and forces resize/fullscreen after resolution or screen change
    // this happens for example when connecting using Remote Desktop
    if (object == QApplication::desktop()) {
        if (event->type() == QEvent::ScreenChangeInternal) {
            showMaximized();
            showFullScreen();
        }
        return false;
    } else if (object == windowHandle()) {
        if (event->type() == QEvent::Resize) {
            QResizeEvent *resize_event = dynamic_cast<QResizeEvent *>(event);
            if (resize_event && resize_event->size() != windowHandle()->screen()->geometry().size()) {
                showMaximized();
                showFullScreen();
            }
        }
        return false;
    } else {
        // forward to default implementation
        return QMainWindow::eventFilter(object, event);
    }
}

void MainWindow::loadPreferences()
{
    QSettings settings;
    
    if (settings.value(kFullscreen, kDefaultFullscreen).toBool()) {
        showFullScreen();
        QApplication::desktop()->installEventFilter(this);
        windowHandle()->installEventFilter(this);
    } else {
        restoreGeometry(settings.value(kWindowGeometry).toByteArray());
        restoreState(settings.value(kWindowState).toByteArray());
    }
    
	_check_priority = settings.value(kCheckPriority, kDefaultCheckPriority).toBool();

    if (_current_access_level < UserManagement::instance().getBaseAccessLevel())
        setAccessLevel();

    _logout_timer->setInterval(settings.value(kLogoutTimeout, kDefaultLogoutTimeout).toInt());
    if (_logout_timer->interval()) {
        _logout_timer->start();
    } else {
        _logout_timer->stop();
    }

    // make sure all changes are reflected in UI
    updateUi();
}

void MainWindow::storePreferences()
{
    QSettings settings;

    if (!isFullScreen()) {
        settings.setValue(kWindowGeometry, saveGeometry());
        settings.setValue(kWindowState, saveState());
    }
}

void MainWindow::showMenu()
{
    _menu->show();
}

void MainWindow::takeScreenshot()
{
    _menu->hide();
    
    QPixmap screenshotPixmap = window()->grab();
    QString screenshotPath = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).filePath(tr("Screenshot") + " " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh.mm.ss") + ".png");
    bool result = screenshotPixmap.save(screenshotPath);
    
    if (result) {
        AuditTrail::message(QObject::tr("Took screenshot"));
        
        bool openResult = QDesktopServices::openUrl(QUrl("file://" + screenshotPath));
        if (!openResult) {
            QApplication::beep();
            qWarning() << "Failed to open screenshot with default application:" << screenshotPath;
        }
    } else {
        qWarning() << "Failed save screenshot to:" << screenshotPath;
    }
}

void MainWindow::changeUser()
{
    LoginPopupDialog m(this);
    bool ok = m.exec();
    QString password = m.password();

	if (ok) {
        if (UserManagement::instance().containsProperty(UserManagement::password, qHash(password))) {
            USER user = UserManagement::instance().getUserByProperty(UserManagement::password, qHash(password));
            _status_bar->setUsername(user.name);
            setAccessLevel(user.accessLevel, user.name, user.uuid, user.cardId);
            return;
        }
		else {
			BMessageBox::critical(this, tr("Invalid Password"), tr("The password you have entered is invalid!"), QMessageBox::Ok);
			_status_bar->setUsername(QString());
		}
	}
	else {
		_status_bar->setUsername(QString());
	}

	setAccessLevel(kAccessLevelGuest, "");
}

void MainWindow::changeUser(const QString cardId)
{
	if (!_access_control_preferences->cardIdReceived(cardId)) {

		if (UserManagement::instance().containsProperty(UserManagement::cardId, cardId)) {
			accessStatus(BSimaticIdent::green, 100);
            USER user = UserManagement::instance().getUserByProperty(UserManagement::cardId, cardId);
			_status_bar->setUsername(user.name);
			setAccessLevel(user.accessLevel, user.name, user.uuid, user.cardId);
		}
		else {
			accessStatus(BSimaticIdent::red, 300);
            _status_bar->setUsername(QString());
			setAccessLevel();
		}
	}
}

void MainWindow::setAccessLevel(int level, QString username, QString uuid, QString card_id)
{
	int _base_access_level = UserManagement::instance().getBaseAccessLevel();
    _current_access_level = level < _base_access_level ? _base_access_level : level;
    
    _status_bar->setUsername(username);
    if (_pluginsController) {
        _pluginsController->setCurrentAccessLevel(_current_access_level, username, uuid, card_id);
    }
    
    // restart logout timer
    if (_current_access_level > _base_access_level && _logout_timer->interval()) {
        _logout_timer->start();
    }
    
    bool changed = username != currentUsername();
    if (changed && username.isEmpty()) { AuditTrail::message(QObject::tr("Logout")); }
    MainWindowInterface::setAccessLevel(this, _current_access_level, username, uuid, card_id);
    if (changed && !username.isEmpty()) { AuditTrail::message(QObject::tr("Login")); }
    
    updateUi();
}

void MainWindow::setPluginsController(PluginsController *pluginsController) {
    _pluginsController = pluginsController;
    _alarm_widget = new AlarmWidget(_pluginsController->alarmDatabase()->tableModel(), this);
    _preferences_widget = new InspectorPreferencesWidget(_pluginsController, this);
    _machine_state_widget = new MachineStateWidget(_pluginsController, this);
    
    // connect plugins controller
    connect(_pluginsController, &PluginsController::machineStateChanged, this, &MainWindow::updateUi);
    connect(_pluginsController, &PluginsController::productNameChanged, _status_bar, &StatusBarWidget::setProduct);
    connect(_pluginsController, &PluginsController::canShowProductWindowChanged, _status_bar, &StatusBarWidget::setCanShowProductWindow);
}

void MainWindow::updateUi()
{
    if (_pluginsController) {
        _status_bar->setMachineState(_pluginsController->machineState(), _pluginsController->diagState());
    }
    
    // cleanup tabs
    if (_pluginsController) {
        auto i = _section_widgets.constBegin();
        while (i != _section_widgets.constEnd()) {
            int before_cnt = i.value()->count();
            i.value()->updateUi(_pluginsController, i.key(), _current_access_level, currentUsername(), currentUuid(), currentCardId());
            
            // check if first level navigation items should be hidden
            int cnt =  i.value()->count();
            if (before_cnt == 0 && cnt > 0) {
                _first_level_navigation->showButton(i.key());
            } else if (before_cnt > 0 && cnt == 0) {
                _first_level_navigation->hideButton(i.key());
                if (_stack_widget->currentWidget() == i.value()) {
                    _stack_widget->setCurrentWidget(_section_widgets[MainWindowInterface::Application]);
                }
            }
            
            ++i;
        }
    }
}

void MainWindow::terminateRequest()
{
    AuditTrail::message(QObject::tr("Quit Application"));
    
    _menu->hide();
    _pluginsController->setRequestedMachineState(Terminate);
    changeToFirstApplicationWidget();
}

void MainWindow::changeToFirstApplicationWidget()
{
    auto tab = _section_widgets[MainWindowInterface::Application];
    tab->setCurrentIndex(0);
    _stack_widget->setCurrentWidget(tab);
}

void MainWindow::setupSystemTrayIcon()
{
    QAction* maximizeAction = new QAction(tr("Show"), this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    QMenu* trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(maximizeAction);

    _trayIcon = new QSystemTrayIcon(this);
    _trayIcon->setContextMenu(trayIconMenu);
    _trayIcon->setIcon(QPixmap(":/images/icon.ico"));
    _trayIcon->show();

    connect(_trayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
        switch (reason) {
        case QSystemTrayIcon::Trigger: showMaximized();
        }
    });
}
