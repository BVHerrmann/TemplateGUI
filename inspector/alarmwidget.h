#ifndef ALARMWIDGET_H
#define ALARMWIDGET_H

#include <QtGui>
#include <QtWidgets>

#include "alarmdatabase.h"


class AlarmWidget : public QWidget
{
Q_OBJECT
public:
    explicit AlarmWidget(QSqlTableModel *model, QWidget *parent = 0);

    void showEvent(QShowEvent *event);
signals:

public slots:

private:
    void setupUi();

    QSqlTableModel *_model;
    QTableView *_table_view;
    bool _auto_scroll;
};

#endif // ALARMWIDGET_H
