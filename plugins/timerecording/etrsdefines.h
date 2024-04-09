#ifndef ETRSDEFINES_H
#define ETRSDEFINES_H

#include <QString>

typedef enum { NoClockStatus, ClockedIn = 1, ClockedOut = 2 } ClockStatus;

const QString ketrsDevice = "ETRS/Devices";
const QString kPNDriverHardwareConfigurationPath = "ETRS/PNDriver/HardwareConfigurationPath";
const QString kPNDriverNetworkInterface = "ETRS/PNDriver/NetworkInterface";
const QString kWorkingTimeRecordingVariante = "Plugins/TimeRecording/Variante";
const QString kUseTestSystem = "Plugins/TimeRecording/UseTestSystem";

const int kVarianteWorkingTime = 1;
const int kVarianteWorkingTimeAndProjectTime = 2;

class ProjectStatusData
{
  public:
    ClockStatus clockStatus = NoClockStatus;
    QString startTime = QString();
    QString projectTaskName = QString();
    QString projectTaskID = QString();
    QString projectName = QString();
    QString date = QString();
    QString productID = QString();
    QString projectID = QString();
    QString workDescription = QString();
};


#endif  // ETRSDEFINES_H
