#include "firstlevelnavigationwidget.h"

#include <QtWidgets>


FirstLevelNavigationWidget::FirstLevelNavigationWidget(QWidget *parent) : BackgroundWidget(parent)
{
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    QImage logoImage(":/images/bertram.svg");
    
    QLabel *logo = new QLabel();
    logo->setObjectName("logo");
    logo->setPixmap(QPixmap::fromImage(logoImage).scaled(250, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logo);
    
    layout->addSpacing(56);
    
    _buttons[MainWindowInterface::Application] = new QPushButton(tr("Application"));
    connect(_buttons[MainWindowInterface::Application], &QPushButton::clicked, [=]() { emit buttonClicked(MainWindowInterface::Application); });
    layout->addWidget(_buttons[MainWindowInterface::Application]);
    
    _buttons[MainWindowInterface::Statistics] = new QPushButton(tr("Statistics"));
    _buttons[MainWindowInterface::Statistics]->hide();
    connect(_buttons[MainWindowInterface::Statistics], &QPushButton::clicked, [=]() { emit buttonClicked(MainWindowInterface::Statistics); });
    layout->addWidget(_buttons[MainWindowInterface::Statistics]);
    
    _buttons[MainWindowInterface::Messages] = new QPushButton(tr("Messages"));
    _buttons[MainWindowInterface::Messages]->hide();
    connect(_buttons[MainWindowInterface::Messages], &QPushButton::clicked, [=]() { emit buttonClicked(MainWindowInterface::Messages); });
    layout->addWidget(_buttons[MainWindowInterface::Messages]);
    
    _buttons[MainWindowInterface::Diagnostics] = new QPushButton(tr("Diagnostics"));
    _buttons[MainWindowInterface::Diagnostics]->hide();
    connect(_buttons[MainWindowInterface::Diagnostics], &QPushButton::clicked, [=]() { emit buttonClicked(MainWindowInterface::Diagnostics); });
    layout->addWidget(_buttons[MainWindowInterface::Diagnostics]);
    
    _buttons[MainWindowInterface::Settings] = new QPushButton(tr("Settings"));
    _buttons[MainWindowInterface::Settings]->hide();
    connect(_buttons[MainWindowInterface::Settings], &QPushButton::clicked, [=]() { emit buttonClicked(MainWindowInterface::Settings); });
    layout->addWidget(_buttons[MainWindowInterface::Settings]);
    
    layout->addStretch();
    
    QHBoxLayout *actionLayout = new QHBoxLayout();

    QPushButton *screenshotButton = new QPushButton("M");
    screenshotButton->setProperty(kRequiredAccessLevel, kAccessLevelUser);
    screenshotButton->setObjectName("screenshot");
    connect(screenshotButton, &QPushButton::clicked, [=]() { emit takeScreenshot(); });
    actionLayout->addWidget(screenshotButton);
    
    actionLayout->addStretch();
    
    QPushButton *quit = new QPushButton("p");
    quit->setProperty(kRequiredAccessLevel, kAccessLevelService);
    quit->setObjectName("quit");
    connect(quit, &QPushButton::clicked, this, &FirstLevelNavigationWidget::quit);
    //connect(quit, &QPushButton::clicked, qApp, &QApplication::quit);
    actionLayout->addWidget(quit);
    
    layout->addLayout(actionLayout);
}

void FirstLevelNavigationWidget::hideButton(const MainWindowInterface::WidgetType &type)
{
    _buttons[type]->hide();
}

void FirstLevelNavigationWidget::showButton(const MainWindowInterface::WidgetType &type)
{
    _buttons[type]->show();
}
