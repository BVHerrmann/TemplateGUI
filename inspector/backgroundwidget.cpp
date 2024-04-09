#include "backgroundwidget.h"

#include <QtWidgets>


BackgroundWidget::BackgroundWidget(QWidget *parent) : QWidget(parent)
{

}

void BackgroundWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
