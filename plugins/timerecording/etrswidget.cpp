#include "etrswidget.h"

#include <colors.h>
#include <contentboarditem.h>
#include <qsvgrenderer.h>

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "etrsplugin.h"
#include "projectwidget.h"

MessageDialog::MessageDialog(QWidget* parent) : QDialog(parent)
{
}

MessageDialog::~MessageDialog()
{
}

void MessageDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
        QDialog::hide();
    else {
        e->ignore();
    }
}

TimeEventWidget::TimeEventWidget(QWidget* parent)
{
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);

    layout->addStretch();

    _employeeName = new QLabel(this);
    _employeeName->setStyleSheet(QString("QLabel {font-size: 48px; font-weight: bold;}"));
    _employeeName->setMinimumSize(0, 56);
    _employeeName->setMaximumSize(maximumWidth(), 56);
    layout->addWidget(_employeeName, 0, Qt::AlignCenter);

    _time = new QLabel(this);
    _time->setStyleSheet(QString("QLabel {font-size: 28px; font-weight: bold;}"));
    _time->setMinimumSize(0, 56);
    _time->setMaximumSize(maximumWidth(), 56);
    layout->addWidget(_time, 0, Qt::AlignCenter);

    layout->addSpacing(128);

    _clockMessage = new QLabel(this);
    _clockMessage->setFixedHeight(250);
    _clockMessage->setStyleSheet(QString("QLabel {font-size: 24px; font-weight: bold;}"));
    _clockMessage->setWordWrap(true);
    layout->addWidget(_clockMessage, 0, Qt::AlignCenter);

    layout->addStretch();

    _messageDurationTimer = new QTimer(this);
    _messageDurationTimer->setSingleShot(true);
    connect(_messageDurationTimer, &QTimer::timeout, this, &TimeEventWidget::hideTimeEvent);

    _loginLogo = QPixmap::fromImage(QImage(":/images/login.svg")).scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _logoutLogo = QPixmap::fromImage(QImage(":/images/logout.svg")).scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void TimeEventWidget::showTimeEventWithMessage(const QString employeeName, const QDateTime time, const QString message)
{
    clearEmployeeViewData();
    _employeeName->setText(employeeName);
    _time->setText(time.toString("hh:mm:ss"));
    _clockMessage->setText(message);
    raise();

    _messageDurationTimer->start(5000);
}

void TimeEventWidget::showLoginTimeEvent(const QString employeeName, const QDateTime time)
{
    clearEmployeeViewData();
    _employeeName->setText(employeeName);
    _time->setText(time.toString("hh:mm:ss"));
    _clockMessage->setPixmap(_loginLogo);
    raise();

    _messageDurationTimer->start(5000);
}

void TimeEventWidget::showLogoutTimeEvent(const QString employeeName, const QDateTime time)
{
     clearEmployeeViewData();
    _employeeName->setText(employeeName);
    _time->setText(time.toString("hh:mm:ss"));
    _clockMessage->setPixmap(_logoutLogo);
    raise();

    _messageDurationTimer->start(5000);
}

void TimeEventWidget::hideTimeEvent()
{
    clearEmployeeViewData();
    emit timeEventClosed();
}

void TimeEventWidget::clearEmployeeViewData()
{
    _employeeName->setText("");
    _time->setText("");
    _clockMessage->setText("");

    QImage empty(250, 250, QImage::Format_ARGB32);
    empty.fill(QColor(255, 255, 255, 0));

    _clockMessage->setPixmap(QPixmap::fromImage(empty));
}


ETRSWidget::ETRSWidget(ETRSPlugin* plugin, bool showStandardView, QWidget* parent) : QWidget(parent)
{
    _projectWidget = nullptr;
    _timeEventWidget = nullptr;
    _mainLayout = nullptr;
    _timeEventDialog = nullptr;
    _dialogEvent = nullptr;
    _plugin = plugin;
    _showStandardView = showStandardView;
    _analogClock = nullptr;
    _projectWidget = nullptr;
    setAttribute(Qt::WA_Hover, true);
    setupUi();
}

ETRSWidget::~ETRSWidget()
{
}

void ETRSWidget::setShowStandardView(bool set, const QString& employeesID, const QString& employeesName)
{
    if (set) {
        if (!_showStandardView) {
            if (!_analogClock) {
                _analogClock = new AnalogClock(this);
            }
            _mainLayout->replaceWidget(_projectWidget, _analogClock);
            delete _projectWidget;
            _projectWidget = nullptr;
            _showStandardView = true;
        }
    } else {
        if (_showStandardView) {
            if (!_projectWidget) {
                _projectWidget = new ProjectWidget(this, _plugin->useTestSystem());
                _projectWidget->updateWorkList(employeesID, employeesName);
            }
            _mainLayout->replaceWidget(_analogClock, _projectWidget);
            delete _analogClock;
            _analogClock = nullptr;
            _showStandardView = false;
        }
    }
    
}

void ETRSWidget::setupUi()
{
    
    _mainLayout = new QGridLayout(this);
    setLayout(_mainLayout);

    DateTimeWidget* dateTimeWidget = new DateTimeWidget(this);
    dateTimeWidget->setMaximumSize(width(), 70);
    _mainLayout->addWidget(dateTimeWidget, 0, 0);

    if (_showStandardView) {
        _analogClock = new AnalogClock(this);
        _mainLayout->addWidget(_analogClock, 0, 1, 2, 1);
    } else {
        _projectWidget = new ProjectWidget(this, _plugin->useTestSystem());
        _mainLayout->addWidget(_projectWidget, 0, 1, 2, 1);
    }
    
    _timeEventWidget = new TimeEventWidget(this);
    _mainLayout->addWidget(_timeEventWidget, 1, 0);
    connect(_timeEventWidget, &TimeEventWidget::timeEventClosed, _plugin, &ETRSPlugin::setAccessLevelGuest);
    
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        _timeEventDialog = new MessageDialog(this);
        _timeEventDialog->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);

        QVBoxLayout* layout = new QVBoxLayout();
        _dialogEvent = new TimeEventWidget();
        _dialogEvent->setMinimumSize(400, 400);
        connect(_dialogEvent, &TimeEventWidget::timeEventClosed, _timeEventDialog, &QDialog::hide);
       
        layout->addWidget(_dialogEvent);
        _timeEventDialog->setLayout(layout);
        
    }
    
    
}

void ETRSWidget::handleLog(const QString employeeName, const QDateTime time, const QString message)
{
    if (_timeEventWidget) {
        _timeEventWidget->showTimeEventWithMessage(employeeName, time, message);
    }
   
    if (QSystemTrayIcon::isSystemTrayAvailable() && window()->isHidden()) {
        _dialogEvent->showTimeEventWithMessage(employeeName, time, message);
        if (_timeEventDialog->isHidden() && !message.isEmpty()) {
            _timeEventDialog->show();
        }
    }
}

void ETRSWidget::handleLogin(const QString employeeName, const QDateTime time)
{
    if (_timeEventWidget) {
        _timeEventWidget->showLoginTimeEvent(employeeName, time);
    }

    if (QSystemTrayIcon::isSystemTrayAvailable() && window()->isHidden()) {
        _dialogEvent->showLoginTimeEvent(employeeName, time);
        if (_timeEventDialog->isHidden()) {
            _timeEventDialog->show();
        }
    }
}

void ETRSWidget::handleLogout(const QString employeeName, const QDateTime time)
{
    if (_timeEventWidget) {
        _timeEventWidget->showLogoutTimeEvent(employeeName, time);
    }

    if (QSystemTrayIcon::isSystemTrayAvailable() && window()->isHidden()) {
        _dialogEvent->showLogoutTimeEvent(employeeName, time);
        if (_timeEventDialog->isHidden()) {
            _timeEventDialog->show();
        }
    }
}

AnalogClock::AnalogClock(QWidget* parent) : ContentBoardItem(parent)
{
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&AnalogClock::update));
    timer->start(1000);
}

void AnalogClock::paintEvent(QPaintEvent* event)
{
    ContentBoardItem::paintEvent(event);

    static const QPoint hourHand[4] = {QPoint(1, -5), QPoint(-1, -5), QPoint(-1, -50), QPoint(1, -50)};
    static const QPoint minuteHand[4] = {QPoint(1, -5), QPoint(-1, -5), QPoint(-1, -80), QPoint(1, -80)};

    QColor hourColor = HMIColor::DarkGrey;
    QColor minuteColor = HMIColor::DarkGrey;

    int side = qMin(width(), height());
    QTime time = QTime::currentTime();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 200.0, side / 200.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(hourColor);
    painter.save();

    painter.rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
    painter.drawConvexPolygon(hourHand, 4);
    painter.restore();
    painter.setPen(hourColor);

    for (int i = 0; i < 12; ++i) {
        painter.drawLine(88, 0, 96, 0);
        painter.rotate(30.0);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(minuteColor);

    painter.save();
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.drawConvexPolygon(minuteHand, 4);
    painter.restore();

    painter.setPen(minuteColor);

    for (int j = 0; j < 60; ++j) {
        if ((j % 5) != 0) painter.drawLine(92, 0, 96, 0);
        painter.rotate(6.0);
    }
}

DateTimeWidget::DateTimeWidget(QWidget* parent) : ContentBoardItem(parent)
{
    QHBoxLayout* layout = new QHBoxLayout();
    setLayout(layout);

    QLabel* date = new QLabel();
    date->setStyleSheet(QString("QLabel {font-size: 24px; font-weight: bold;}"));
    QLabel* time = new QLabel();
    time->setStyleSheet(QString("QLabel {font-size: 24px; font-weight: bold;}"));

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=]() {
        date->setText(QDate::currentDate().toString("dd.MM.yyyy"));
        time->setText(QTime::currentTime().toString("hh:mm:ss"));
    });
    timer->start(500);

    layout->addWidget(date);
    layout->addStretch();
    layout->addWidget(time);
}