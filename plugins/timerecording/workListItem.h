#pragma once
#include <QString>
class workListItem
{
  public:
    workListItem(const QString ProjectID, const QString ProjectName, const QString plannedPeriodStartDateTime, const QString plannedPeriodEndDateTime, const QString projectTaskName,
                 const bool _workAreaNecessary);
    ~workListItem();
    bool isEqual(const workListItem& Item) const;
  public:
    QString _projectID = QString();
    QString _projectName = QString();
    QString _plannedPeriodStartDateTime = QString();
    QString _plannedPeriodEndDateTime = QString();
    QString _projectTaskName = QString();
    bool _workAreaNecessary = true;
};
bool operator==(const workListItem& a, const workListItem& b);


