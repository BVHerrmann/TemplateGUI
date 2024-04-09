#pragma once

#include "popupdialog.h"
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include "etrsdefines.h"

class JobDescriptionWidget  : public PopupDialog
{
	Q_OBJECT
public:
	JobDescriptionWidget(QWidget *parent);
	~JobDescriptionWidget();
    void setupUi();
    void setJobDescription(const QString& PlainText);
    void setUserName(const QString& set);
    void setProjectStartTime(const QString& set);
    void setProjectID(const QString& set);
    void setProjectStatus(const QString& set);
    QStringList getJobDescriptionTextLines() { return _jobDescriptionTextLines; }
    void setClockStatus(ClockStatus set);
   
   
signals:
    void signalProjectClockIn();
    void signalProjectClockOut();

  private:
    QTextEdit* _textEditJobDescription;
    QStringList _jobDescriptionTextLines;
    QLabel* _labelUserName;
    QLabel* _labelProjectStartTime;
    QLabel* _labelProjectID;
    QLabel* _labelProjectStatus;
    QDialogButtonBox* _buttonBox;
    ClockStatus _clockStatus;

};
