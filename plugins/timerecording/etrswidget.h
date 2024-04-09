#ifndef ETRSWIDGET_H
#define ETRSWIDGET_H

#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "contentboarditem.h"
#include "employee.h"
#include "etrsplugin.h"






class ProjectWidget;
class MessageDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit MessageDialog(QWidget* parent = 0);
    ~MessageDialog();

  protected:
    void keyPressEvent(QKeyEvent* e) override;
};

class TimeEventWidget : public ContentBoardItem
{
    Q_OBJECT

  public:
    TimeEventWidget(QWidget* parent = nullptr);
    void clearEmployeeViewData();

  signals:
    void timeEventClosed();
    

  public slots:
    void showTimeEventWithMessage(const QString employeeName, const QDateTime time, const QString message);
    void showLoginTimeEvent(const QString employeeName, const QDateTime time);
    void showLogoutTimeEvent(const QString employeeName, const QDateTime time);
    void hideTimeEvent();
    

  private:
    QLabel* _employeeName = nullptr;
    QLabel* _time = nullptr;
    QLabel* _clockMessage = nullptr;
    QPixmap _loginLogo;
    QPixmap _logoutLogo;

    QTimer* _messageDurationTimer;
};
class AnalogClock;
class ETRSWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ETRSWidget(ETRSPlugin* plugin, bool showStandardView = true, QWidget* parent = 0);
    ~ETRSWidget();
    bool isStandardView() { return _showStandardView; }
    void setShowStandardView(bool set, const QString& employeesID, const QString& employeesName);
  
  public slots:
    void handleLog(const QString employeeName, const QDateTime time, const QString message);
    void handleLogin(const QString employeeName, const QDateTime time);
    void handleLogout(const QString employeeName, const QDateTime time);
    
  private:
    void setupUi();
    bool _showStandardView;
    ETRSPlugin* _plugin;
    TimeEventWidget* _timeEventWidget;
    MessageDialog* _timeEventDialog;
    TimeEventWidget* _dialogEvent;
    ProjectWidget* _projectWidget;
    AnalogClock* _analogClock;
    QGridLayout* _mainLayout;
    
};

class AnalogClock : public ContentBoardItem
{
    Q_OBJECT

  public:
    AnalogClock(QWidget* parent = nullptr);

  protected:
    void paintEvent(QPaintEvent* event) override;
};

class DateTimeWidget : public ContentBoardItem
{
    Q_OBJECT

  public:
    DateTimeWidget(QWidget* parent = nullptr);
};





#endif  // ETRSWIDGET_H
