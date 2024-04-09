#ifndef TITLEBARWIDGET_H
#define TITLEBARWIDGET_H

#include "backgroundwidget.h"


class TitleBarWidget : public BackgroundWidget
{
    Q_OBJECT
public:
    explicit TitleBarWidget(QWidget *parent = nullptr);
    
signals:
    void titleClicked();
    void openFirstLevelNavigationClicked();
    
public slots:
    
};

#endif // TITLEBARWIDGET_H
