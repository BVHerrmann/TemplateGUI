#include "workListItem.h"

workListItem::workListItem(const QString ProjectID, const QString ProjectName, const QString plannedPeriodStartDateTime, const QString plannedPeriodEndDateTime, const QString projectTaskName,
                           const bool workAreaNecessary)
{
    _projectID = ProjectID;
    _projectName = ProjectName;
    _plannedPeriodStartDateTime = plannedPeriodStartDateTime;
    _plannedPeriodEndDateTime = plannedPeriodEndDateTime;
    _projectTaskName = projectTaskName;
    _workAreaNecessary = workAreaNecessary;
}

workListItem::~workListItem()
{
}

bool workListItem::isEqual(const workListItem& Item) const
{
    if (_projectName == Item._projectName) {
        return true;
    }
    return false;
}

bool operator==(const workListItem& a, const workListItem& b)
{
    return a.isEqual(b);
}
