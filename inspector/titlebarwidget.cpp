#include "titlebarwidget.h"

#include <QtWidgets>

#include "inspectordefines.h"


TitleBarWidget::TitleBarWidget(QWidget *parent) : BackgroundWidget(parent)
{
    QSettings settings;
    
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QPushButton *title = new QPushButton(settings.value(kCustomAppName, QCoreApplication::applicationName()).toString());
    connect(title, &QPushButton::clicked, this, &TitleBarWidget::titleClicked);
    layout->addWidget(title);
    
    layout->addStretch();
    
    QImage logoImage(":/images/bertram.svg");
  
    QLabel *logo = new QLabel();
    logo->setObjectName("logo");
    logo->setPixmap(QPixmap::fromImage(logoImage).scaled(250, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logo);
    
    QPushButton *burger = new QPushButton("v");
    burger->setObjectName("firstLevelNavigation");
    connect(burger, &QPushButton::clicked, this, &TitleBarWidget::openFirstLevelNavigationClicked);
    layout->addWidget(burger);
}
