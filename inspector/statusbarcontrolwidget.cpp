#include "statusbarcontrolwidget.h"

#include <QtWidgets>


StatusBarControlWidget::StatusBarControlWidget(QString title, QWidget *parent) : BackgroundWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetMaximumSize);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(0);
    
    QLabel *title_label = new QLabel(title);
    title_label->setObjectName("title");
    
    _sub_title_label = new QLabel("");
    _sub_title_label->setObjectName("subTitle");
    
    _button = new QPushButton("w");
    _button->setObjectName("button");
    connect(_button, &QPushButton::clicked, this, &StatusBarControlWidget::clicked);
    
    layout->addWidget(title_label, 0, 0, Qt::AlignTop);
    layout->addWidget(_sub_title_label, 1, 0, Qt::AlignBottom);
    layout->addWidget(_button, 0, 1, 2, 1, Qt::AlignRight);
}

void StatusBarControlWidget::setSubtitle(const QString &title)
{
    _sub_title_label->setText(title);
}

void StatusBarControlWidget::setButtonVisible(bool value)
{
    _button->setVisible(value);
}
