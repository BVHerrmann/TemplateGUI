#ifndef ETRSPLUGIN_H
#define ETRSPLUGIN_H

#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>
#include <interfaces.h>
#include <plugin.h>
#include <pniovalue.h>

#include <QReadWriteLock>

#include "BPNSerialPort.h"
#include "BSimaticident.h"
#include "DoorAccessControl.h"
#include "devicemanagement.h"
#include "timerecordingsystem.h"

// Error Mask
typedef enum {
    noErr = 0,

    errProfiNet = 1 << 0,
    errSinamics = 1 << 1,
    errCamera = 1 << 2,
    errSafetyContact = 1 << 3,
    errAirPressure = 1 << 4,

} Error;

// Warning Mask
typedef enum {
    noWarn = 0,

    warnNoSerialPortFound = 1 << 0,
    warnWrongDeviceConnection = 1 << 1,
    warnWrongSystemType = 1 << 2,
    warnNoCardReaderFound = 1 << 3,
    warnNoHardwareConfigurtaion = 1 << 4,
    warnNoMacAddress = 1 << 5,
    warnMacAddressNotParsed = 1 << 6,
    warnServerRequestError = 1 << 7,

} Warning;


class ETRSWidget;
class ETRSConfigWidget;
class ETRSPlugin : public Plugin, PluginInterface, MainWindowInterface, CommunicationInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.ETRSPlugin")
    Q_INTERFACES(PluginInterface)
    Q_INTERFACES(MainWindowInterface)
    Q_INTERFACES(CommunicationInterface)

  public:
    enum Type { INT8, UINT8, INT16, UINT16, INT32, UINT32, INT64, UINT64, FLOAT32, RAW8, RAW16, RAW32, RAW64 };

    explicit ETRSPlugin(QObject* parent = 0);
    virtual ~ETRSPlugin();

    // PluginInterface
    const QString identifier() const { return "ETRS"; }
    const QString name() const { return tr("Zeiterfassung"); }
    QThread::Priority priority() const { return QThread::NormalPriority; }
    const MachineState machineState() const;
    const DiagState diagState() const;
    void updateMessages();

    // MainWindowInterface
    const WidgetType widgetType(const int idx = 0) const;
    const QString title(const int idx = 0) const;
    QWidget* mainWidget(const int idx = 0)  const;
    int preferredTabIndex(const int idx = 0) const
    {
        Q_UNUSED(idx);
        return 0;
    }
    int requiredWidgetAccessLevel(const int idx = 0) const;
    int mainWidgetCount() const { return 2; }

    int getTimeRecordingVariante() { return _timeRecordingVariante; }
    bool useTestSystem() { return _useTestSystem; }

    void setShowStandardView(bool set,const QString&,const QString&);
    void ReStartTimerShowProjectView();
      
 
  signals:
    // CommunicationInterface
    void valuesChanged(const QHash<QString, QVariant>& values);
    void valueChanged(const QString& name, const QVariant& value);

    void setValuesReady(const QHash<QString, QVariant>& values);

    void setCurrentUser(const QString &card_id);
    void restartTimerShowProjectView();
   

  public slots:
    // PluginInterface
    void initialize();
    void uninitialize();
    void requestMachineState(const PluginInterface::MachineState state) override { _request_state = state; }
    void currentUser(int access_level, QString const& username, QString const& uuid, QString const& card_id);
    void reset();

    // CommunicationInterface
    void setValues(const QHash<QString, QVariant>& values);
    void setValue(const QString& name, const QVariant& value) { defaultSetValue(name, value); }

    void setWarningMask(int set, const QString& WarningText);
    void setAccessLevelGuest();

   

  private:
    PluginInterface::MachineState _request_state = Off;
    // functions
    QHash<QString, QVariant> setupDevice(const DEVICE device);
    QSerialPortInfo getSerialPortCardreader(const QString name);
    void setupConfig();

    // Variables
   
    ETRSWidget* _etrsWidget;
    ETRSConfigWidget* _config_widget;
    QList<AccessControlSystem*> _activeSystems;

    QTimer _switchToStandardView;

    QReadWriteLock _editLock;
    QReadWriteLock _saveLock;

    QString _warningText;
    QString _currentEmployeeID;
    QString _currentEmployeeName;

    int _error_mask;
    int _warning_mask;

    int _timeRecordingVariante;
    bool _useTestSystem;
    bool _showStandardView;

    // Output
    output<bool> q0;
    output<bool> q1;
    output<bool> q2;
    output<bool> q3;
    output<bool> q4;
    output<bool> q5;
    output<bool> q6;
    output<bool> q7;
};

#endif  // ETRSPLUGIN_H
