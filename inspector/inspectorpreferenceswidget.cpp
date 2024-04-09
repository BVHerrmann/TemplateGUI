#include "inspectorpreferenceswidget.h"

#include <QtGui>
#include <QtWidgets>

#include <audittrail.h>

#include "interfaces.h"

#include "inspectordefines.h"
#include "translator.h"


InspectorPreferencesWidget::InspectorPreferencesWidget(PluginsController *pluginsController, QWidget *parent) :
    PreferencesWidget(parent)
{
    _plugins_model = new PluginListModel(pluginsController, this);
    
    setupUi();
}


void InspectorPreferencesWidget::setupUi()
{
    QSettings settings;
    QGridLayout *layout = new QGridLayout();

    // common
    QFormLayout *common_layout = new QFormLayout();
    QGroupBox *common_group = new QGroupBox(tr("Common"));
    common_group->setLayout(common_layout);
    layout->addWidget(common_group, 0, 0);

    // application name
    QLineEdit *applicationName = new QLineEdit();
    applicationName->setProperty(kRequiredAccessLevel, kAccessLevelBertram);
    applicationName->setProperty(kPropertyKey, kCustomAppName);
    applicationName->setProperty(kAuditTrail, tr("Application Name"));
    applicationName->setText(settings.value(kCustomAppName, QCoreApplication::applicationName()).toString());
    connect(applicationName, &QLineEdit::textChanged, this, static_cast<void (PreferencesWidget::*)(const QString &)>(&PreferencesWidget::changeValue));
    common_layout->addRow(tr("Application Name"), applicationName);
    
    // language
    QComboBox *languageComboBox = new QComboBox();
    languageComboBox->setProperty(kPropertyKey, kLocale);
    languageComboBox->setProperty(kAuditTrail, tr("Language"));
    
    QStringList languages = Translator::getInstance()->availableLanguages();
    for (const QString &language : languages) {
        QLocale locale(language);
        languageComboBox->addItem(QLocale::languageToString(locale.language()), language);
    }
    
    for (int i=0; i < languageComboBox->count(); i++) {
        if (languageComboBox->itemData(i).toString() == settings.value(kLocale, "de").toString()) {
            languageComboBox->setCurrentIndex(i);
            break;
        }
    }
    connect(languageComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &InspectorPreferencesWidget::changeLanguage);
    common_layout->addRow(tr("Language"), languageComboBox);

    // access level for mode change
    QComboBox *accessLevel = new QComboBox();
    accessLevel->addItem(tr("Guest"), kAccessLevelGuest);
    accessLevel->addItem(tr("User"), kAccessLevelUser);
    accessLevel->addItem(tr("Service"), kAccessLevelService);
    accessLevel->addItem(tr("SysOp"), kAccessLevelSysOp);
    accessLevel->addItem(tr("Admin"), kAccessLevelAdmin);
    accessLevel->addItem("Bertram", kAccessLevelBertram);

    int index = accessLevel->findData(settings.value(kAllowMachineState, kAccessLevelGuest).toInt());
    accessLevel->setCurrentIndex(index);
    accessLevel->setProperty(kRequiredAccessLevel, kAccessLevelBertram);
    accessLevel->setProperty(kPropertyKey, kAllowMachineState);
    accessLevel->setProperty(kAuditTrail, tr("Allow Machine State change"));
    connect(accessLevel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, static_cast<void (PreferencesWidget::*)(int)>(&PreferencesWidget::changeIndex));
    common_layout->addRow(tr("Allow Machine State change"), accessLevel);
    
    QCheckBox *virtual_keyboard_check_box = new QCheckBox();
    virtual_keyboard_check_box->setProperty(kPropertyKey, kVirtualKeyboard);
    virtual_keyboard_check_box->setProperty(kAuditTrail, tr("Virtual Keyboard"));
    virtual_keyboard_check_box->setChecked(settings.value(kVirtualKeyboard, kDefaultVirtualKeyboard).toBool());
    connect(virtual_keyboard_check_box, &QCheckBox::toggled, this, static_cast<void (PreferencesWidget::*)(bool)>(&PreferencesWidget::changeValue));
    common_layout->addRow(tr("Virtual Keyboard"), virtual_keyboard_check_box);
    
    QCheckBox *full_screen_check_box = new QCheckBox();
    full_screen_check_box->setProperty(kPropertyKey, kFullscreen);
    full_screen_check_box->setProperty(kAuditTrail, tr("Fullscreen Mode"));
    full_screen_check_box->setChecked(settings.value(kFullscreen, kDefaultFullscreen).toBool());
    connect(full_screen_check_box, &QCheckBox::toggled, this, static_cast<void (PreferencesWidget::*)(bool)>(&PreferencesWidget::changeValue));
    common_layout->addRow(tr("Fullscreen Mode"), full_screen_check_box);
    
	QCheckBox *sytem_tray_icon_check_box = new QCheckBox();
    sytem_tray_icon_check_box->setProperty(kPropertyKey, kSystemTrayIcon);
	sytem_tray_icon_check_box->setChecked(settings.value(kSystemTrayIcon, kDefaultSystemTrayIcon).toBool());
    connect(sytem_tray_icon_check_box, &QCheckBox::toggled, this, static_cast<void (PreferencesWidget::*)(bool)>(&PreferencesWidget::changeValue));
    common_layout->addRow(tr("Minimize Mode"), sytem_tray_icon_check_box);

    QCheckBox *app_priority_check_box = new QCheckBox();
    app_priority_check_box->setProperty(kPropertyKey, kCheckPriority);
    full_screen_check_box->setProperty(kAuditTrail, tr("Check Application Priority"));
    app_priority_check_box->setChecked(settings.value(kCheckPriority, kDefaultCheckPriority).toBool());
    connect(app_priority_check_box, &QCheckBox::toggled, this, static_cast<void (PreferencesWidget::*)(bool)>(&PreferencesWidget::changeValue));
    common_layout->addRow(tr("Check Application Priority"), app_priority_check_box);
    
    // plugins
    QVBoxLayout *plugins_layout = new QVBoxLayout();
    QGroupBox *plugins_group = new QGroupBox(tr("Plugins"));
    plugins_group->setProperty(kRequiredAccessLevel, kAccessLevelBertram);
    plugins_group->setLayout(plugins_layout);
    layout->addWidget(plugins_group, 0, 1, 2, 1);
    
    QListView *plugins_list_view = new QListView(this);
    plugins_list_view->setModel(_plugins_model);
    connect(plugins_list_view, &QListView::clicked, _plugins_model, &PluginListModel::toggleItemAtIndex);
    plugins_layout->addWidget(plugins_list_view);
    
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(1, 1);
    
    this->setLayout(layout);
}

void InspectorPreferencesWidget::changeLanguage(int index)
{
    QComboBox *languageComboBox = (QComboBox *)QObject::sender();
    if (languageComboBox) {
        changeValue(languageComboBox, languageComboBox->itemData(index));
        
        Translator::getInstance()->updateTranslations();
    }
}
