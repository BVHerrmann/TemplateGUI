#ifndef BACKGROUNDWIDGET_H
#define BACKGROUNDWIDGET_H

#include <QWidget>

class BackgroundWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackgroundWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event);
};

#endif // BACKGROUNDWIDGET_H
