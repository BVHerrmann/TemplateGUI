#include "JobDescriptionWidget.h"
#include <QtWidgets>
#include "bmessagebox.h"
#include "contentboarditem.h"

JobDescriptionWidget::JobDescriptionWidget(QWidget* parent) : PopupDialog(parent)
{
    _clockStatus = ClockStatus::ClockedOut;
    setupUi();
}

JobDescriptionWidget::~JobDescriptionWidget()
{
}

void JobDescriptionWidget::setupUi()
{
    QString FontSize = "{font-size : 28px; font-weight : bold;}";

    QGridLayout* gridLayoutMain = new QGridLayout(this);
    gridLayoutMain->setSpacing(6);
    gridLayoutMain->setContentsMargins(11, 11, 11, 11);
   
    QGridLayout* gridLayoutWidgetLeftAndRight = new QGridLayout();
    gridLayoutWidgetLeftAndRight->setSpacing(6);

    ContentBoardItem* widgetProjectUserName = new ContentBoardItem(this);
    QGridLayout* gridLayoutProjectUserName = new QGridLayout(widgetProjectUserName);
    gridLayoutProjectUserName->setSpacing(6);
    gridLayoutProjectUserName->setContentsMargins(100, 100, 100, 100);
    _labelUserName = new QLabel(widgetProjectUserName);
    _labelUserName->setStyleSheet(QString("QLabel %1").arg(FontSize));
    _labelProjectStartTime = new QLabel(widgetProjectUserName);
    _labelProjectStartTime->setStyleSheet(QString("QLabel %1").arg(FontSize));
    _labelProjectID = new QLabel(widgetProjectUserName);
    _labelProjectID->setStyleSheet(QString("QLabel %1").arg(FontSize));
    _labelProjectStatus = new QLabel(widgetProjectUserName);
    _labelProjectStatus->setStyleSheet(QString("QLabel %1").arg(FontSize));

    gridLayoutProjectUserName->addWidget(_labelProjectID, 0, 0, 1, 1);
    gridLayoutProjectUserName->addWidget(_labelUserName, 1, 0, 1, 1);
    gridLayoutProjectUserName->addWidget(_labelProjectStartTime, 2, 0, 1, 1);
    gridLayoutProjectUserName->addWidget(_labelProjectStatus, 3, 0, 1, 1);
    gridLayoutWidgetLeftAndRight->addWidget(widgetProjectUserName, 0, 0, 1, 1);

    ContentBoardItem* widgetProjectTextArea = new ContentBoardItem(this);
    QGridLayout* gridLayoutProjectTextArea = new QGridLayout(widgetProjectTextArea);
    gridLayoutProjectTextArea->setSpacing(6);
    gridLayoutProjectTextArea->setContentsMargins(11, 11, 11, 11);

    QGroupBox* groupBoxJobDescription = new QGroupBox(widgetProjectTextArea);
    QGridLayout* gridLayoutGroupBoxJobDescription = new QGridLayout(groupBoxJobDescription);
    gridLayoutGroupBoxJobDescription->setSpacing(6);
    gridLayoutGroupBoxJobDescription->setContentsMargins(11, 11, 11, 11);
    _textEditJobDescription = new QTextEdit(groupBoxJobDescription);
    _textEditJobDescription->setStyleSheet(QString("QTextEdit %1").arg(FontSize));
    groupBoxJobDescription->setStyleSheet(QString("QGroupBox::title {font-size: 24px;}"));
    gridLayoutGroupBoxJobDescription->addWidget(_textEditJobDescription, 0, 0, 1, 1);

    //QPushButton* pushButtonClockout = new QPushButton(groupBoxJobDescription);
    //pushButtonClockout->setStyleSheet(QString("QPushButton %1").arg(FontSize));
    //gridLayoutGroupBoxJobDescription->addWidget(pushButtonClockout, 1, 0, 1, 1);

    gridLayoutProjectTextArea->addWidget(groupBoxJobDescription, 0, 0, 1, 1);

    gridLayoutWidgetLeftAndRight->addWidget(widgetProjectTextArea, 0, 1, 1, 1);

    gridLayoutMain->addLayout(gridLayoutWidgetLeftAndRight, 0, 0, 1, 1);

    _buttonBox = new QDialogButtonBox(this);
    
    _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    gridLayoutMain->addWidget(_buttonBox, 1, 0, 1, 1);

    //buttonBox->setStyleSheet(QString("QPushButton %1").arg(FontSize));

    _buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Abbruch"));
    if (_clockStatus == ClockStatus::ClockedOut) {
        _buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Einstempeln"));
    } else {
        _buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ausstempeln"));
    }

    //connect(pushButtonClockout, &QPushButton::clicked, [=]() {
    //      emit signalProjectClockOut();
    //    });

    connect(_buttonBox, &QDialogButtonBox::clicked, [=](QAbstractButton* button) {
        QString text;
        switch (_buttonBox->standardButton(button)) {
            case QDialogButtonBox::Cancel:
                accept();
                this->close();
                break;
            case QDialogButtonBox::Ok:
                text = _textEditJobDescription->toPlainText();
                _jobDescriptionTextLines = text.split(QRegExp("[\n]"), Qt::SkipEmptyParts);
                if (_jobDescriptionTextLines.count() == 0) {
                    BMessageBox* msgBox = new BMessageBox(QMessageBox::Critical, tr("Fehler"), tr("  Arbeitsbeschreibung fehlt!"), this);
                    msgBox->setStyleSheet(QString("QLabel %1").arg(FontSize));
                    msgBox->addButton(QMessageBox::Ok);
                    msgBox->show();
                } else {
                    reject();
                    this->close();
                    if (_clockStatus == ClockStatus::ClockedOut) {
                        emit signalProjectClockIn();
                    } else {
                        emit signalProjectClockOut();
                    }
                }
                break;
            default:
                break;
        }
    });
    _labelUserName->setText("Name: ");
    _labelProjectStartTime->setText("Begin: ");
    _labelProjectID->setText("Projekt: ");
    groupBoxJobDescription->setTitle("Arbeitsbeschreibung");
    //pushButtonClockout->setText("Ausstempeln");
     centralWidget()->setLayout(gridLayoutMain);
}

void JobDescriptionWidget::setClockStatus(ClockStatus set)
{
    _clockStatus = set;
    if (_clockStatus == ClockStatus::ClockedOut) {
        _buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Einstempeln"));
    } else {
        _buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ausstempeln"));
    }
}

void JobDescriptionWidget::setJobDescription(const QString &PlainText)
{
    if (_textEditJobDescription) {
        _textEditJobDescription->clear();
        _textEditJobDescription->insertPlainText(PlainText);
    }
}

void JobDescriptionWidget::setUserName(const QString& set)
{
    if (_labelUserName) {
         QString userName = "Name: " + set;
        _labelUserName->setText(userName);
    }
}

void JobDescriptionWidget::setProjectStartTime(const QString& set)
{
    if (_labelProjectStartTime) {
        QString setStartProjectTime = "Begin: " + set;
        setStartProjectTime.replace(QString("T"), QString(" "));
        setStartProjectTime.replace(QString("Z"), QString(""));
        _labelProjectStartTime->setText(setStartProjectTime);
    }
}

void JobDescriptionWidget::setProjectID(const QString& set)
{
    if (_labelProjectID) {
        QString projectName = "Projekt: " + set;
        _labelProjectID->setText(projectName);
    } 
}

void JobDescriptionWidget::setProjectStatus(const QString& set)
{
    if (_labelProjectStatus) {
        QString projectName = "Status: " + set;
        _labelProjectStatus->setText(projectName);
    }
}


