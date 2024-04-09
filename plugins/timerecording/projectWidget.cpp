#include "projectWidget.h"
#include "erpclient.h"
#include "JobDescriptionWidget.h"

ProjectWidget::ProjectWidget(QWidget* parent, bool useTestsystem) : QWidget(parent)
{
    _erpClient = new ErpClient(this);
    _erpClient->setUseTestSystem(useTestsystem);
    _employeeID = "";
    _employeeName = "";
    setupUi();
    connect(_listViewWorkList, &QListWidget::itemClicked, this, &ProjectWidget::slotItemClicked);
    connect(_jobDescriptionWidget, &JobDescriptionWidget::signalProjectClockIn, this, &ProjectWidget::slotProjectClockIn);
    connect(_jobDescriptionWidget, &JobDescriptionWidget::signalProjectClockOut, this, &ProjectWidget::slotProjectClockOut);
}

ProjectWidget::~ProjectWidget()
{
}

void ProjectWidget::slotProjectClockIn()
{
    if (_jobDescriptionWidget) {
        QStringList JobDescriptionTextLines = _jobDescriptionWidget->getJobDescriptionTextLines();
        // write jobJobDescriptionTextLines
    }
}

void ProjectWidget::slotProjectClockOut()
{

}

void ProjectWidget::checkClockStatus(const QString& employeeID, const QString& employeeName)
{
    _erpClient->getProjectStatus(employeeID, [=](ProjectStatusData psd, QString errorMsg) {

    });
}

void ProjectWidget::updateWorkList(const QString& employeeID, const QString& employeeName)
{
    _employeeID = employeeID;
    _employeeName = employeeName;
    _erpClient->getEmployeeWorklist(_employeeID, [=](QList<workListItem> workList,QString errorMsg) {
        if (!workList.isEmpty()  && workList != _workList) {
            _workList = workList; 
             viewWorklist();
        }
        if (!errorMsg.isEmpty()) {
            qWarning() << errorMsg;
        }
    });

    _erpClient->getProjectStatus(employeeID, [=](ProjectStatusData psd, QString errorMsg) { 
        if (psd.clockStatus == ClockStatus::ClockedOut) {
            _jobDescriptionWidget->setUserName(_employeeName);
            _jobDescriptionWidget->setProjectStartTime(psd.startTime);
            _jobDescriptionWidget->setProjectID(psd.projectID);
            _jobDescriptionWidget->setJobDescription(psd.workDescription);
            _jobDescriptionWidget->setProjectStatus("Engestempelt");
            _jobDescriptionWidget->setClockStatus(ClockStatus::ClockedIn);
            _jobDescriptionWidget->show();
        }
    });
}

void ProjectWidget::slotItemClicked(QListWidgetItem* item)
{
    _currentWorkListIndex = -1;
    int index = _listViewWorkList->row(item);
    if (index >= 0 && index < _workList.count()) {
        _currentWorkListIndex = index;
        if (_workList.at(_currentWorkListIndex)._workAreaNecessary) {
             QString text = QStringLiteral("Bedienoberfläche anpassen");
            
            _jobDescriptionWidget->setUserName(_employeeName);
            _jobDescriptionWidget->setProjectStartTime(_workList.at(_currentWorkListIndex)._plannedPeriodStartDateTime);
            _jobDescriptionWidget->setProjectID(_workList.at(_currentWorkListIndex)._projectID);
            _jobDescriptionWidget->setJobDescription(text);
            _jobDescriptionWidget->show();
        } else {
            _dialogProjectInOut->show();
        }
    }
    qDebug() << "row [" << index << "] == " << item->text();
}

void ProjectWidget::viewWorklist()
{
    _listViewWorkList->clear();
    int i = 0;
    for (auto workItem : _workList) {
        i++;
        QWidget* w = new QWidget(_listViewWorkList);
        QHBoxLayout* layout = new QHBoxLayout(w);
        QLabel* projectInfo = new QLabel(_listViewWorkList);
        projectInfo->setStyleSheet(QString("QLabel {font-size: 28px; font-weight: bold;}"));
        QString TextInfo = workItem._projectID + " " + workItem._projectTaskName + "\n" + QString("      Startzeit:%1").arg(workItem._plannedPeriodStartDateTime) + " " +
                           QString("Restzeit:%1 ").arg(workItem._plannedPeriodEndDateTime);
        projectInfo->setAlignment(Qt::AlignLeft);
        if (i < 10) {
            projectInfo->setText(QString("%1.   %2").arg(i).arg(TextInfo));
        } else {
            projectInfo->setText(QString("%1. %2").arg(i).arg(TextInfo));
        }
        layout->addWidget(projectInfo);
        QListWidgetItem* qlistwidgetitem = new QListWidgetItem;
        QSize s = w->sizeHint();
        s.rheight() += 5;
        qlistwidgetitem->setSizeHint(s);
        _listViewWorkList->addItem(qlistwidgetitem);
        _listViewWorkList->setItemWidget(qlistwidgetitem, w);
    }
}

void ProjectWidget::setupUi()
{
    _jobDescriptionWidget = new JobDescriptionWidget(this);
    _dialogProjectInOut = new DialogStampIn(this);

    QGridLayout* gridLayout = new QGridLayout(this);
    ContentBoardItem* widgetProjectList = new ContentBoardItem(this);
    QGridLayout* gridLayoutProjectList = new QGridLayout(widgetProjectList);
    _listViewWorkList = new QListWidget(widgetProjectList);
    _listViewWorkList->setLayoutDirection(Qt::RightToLeft);
    gridLayoutProjectList->addWidget(_listViewWorkList,0, 0, 1, 1);

    ProjectHeaderWidget* projectHeaderWidget = new ProjectHeaderWidget(this);
    projectHeaderWidget->setMaximumSize(16777215, 70);
    gridLayout->setContentsMargins(2, 2, 2, 2);
    gridLayout->addWidget(projectHeaderWidget, 0, 0, 1, 1);
    gridLayout->addWidget(widgetProjectList, 1, 0, 1, 1);
}

ProjectHeaderWidget::ProjectHeaderWidget(QWidget* parent) : ContentBoardItem(parent)
{
    QHBoxLayout* layout = new QHBoxLayout();
    setLayout(layout);
    QLabel* projectTitle = new QLabel(this);
    projectTitle->setStyleSheet(QString("QLabel {font-size: 28px; font-weight: bold;}"));
    projectTitle->setAlignment(Qt::AlignCenter);
    projectTitle->setText("Projekt Liste");
    layout->addWidget(projectTitle, 0, 0);
}


