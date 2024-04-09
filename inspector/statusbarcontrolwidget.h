#ifndef STATUSBARCONTROLWIDGET_H
#define STATUSBARCONTROLWIDGET_H

#include "backgroundwidget.h"

#include <QtWidgets>


class StatusBarControlWidget : public BackgroundWidget
{
    Q_OBJECT
    
public:
    explicit StatusBarControlWidget(QString title, QWidget *parent = nullptr);
    
    void setSubtitle(const QString &title);
    void setButtonVisible(bool value);
    
signals:
    void clicked();
    
public slots:
    
private:
    QLabel *_sub_title_label;
    QPushButton *_button;
};

#endif // STATUSBARCONTROLWIDGET_H
