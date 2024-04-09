#ifndef PLUGINSCONTROLLER_H
#define PLUGINSCONTROLLER_H


namespace cv {
    class Mat;
}

class InspectionResult;

#include "interfaces.h"
#include "alarmdatabase.h"
class MainWindow;


class PluginsController : public QObject
{
    Q_OBJECT

public:
    explicit PluginsController(MainWindow *mainWindow, QObject *parent = 0);
    virtual ~PluginsController();

    void loadAllPlugins();
    QObjectList availablePlugins();
    QObjectList loadedPlugins() { return _loaded_plugins; }

    bool isPluginEnabled(QObject *object);
    void setPluginEnabled(QObject *object, bool value);

    QString pluginIdentifier(QObject *object);
    QString pluginName(QObject *object);
    
    PluginInterface::MachineState machineState() const { return _current_machine_state; }
    PluginInterface::DiagState diagState() const { return _current_diag_state; }
    QList<PluginInterface::Message> messages() { QReadLocker locker(&_messages_lock); return QList<PluginInterface::Message>(_messages); }
    QString productName() const;
    
    AlarmDatabase *alarmDatabase() const { return _db; }
    
    bool canShowProductWindow() const { return _can_show_product_window; };
    void showProductWindow();
    
    void setCurrentAccessLevel(uint accessLevel, QString const& username, QString const& uuid, QString const& card_id)
    {
        _current_access_level = accessLevel;
        emit userChanged(accessLevel, username, uuid, card_id);
    }
    
    PluginInterface::MachineState requestedMachineState() const { return _requested_machine_state; }
    
signals:
    // PluginInterface
    void initializePlugins();
    void uninitializePlugins();
    void resetPlugins();
    void localeChanged(const QString &locale);
    void requestedMachineStateChanged(const PluginInterface::MachineState &state);
    void machineStateChanged(const PluginInterface::MachineState &machineState, const PluginInterface::DiagState &diagState);
    void messagesChanged(const QList<PluginInterface::Message> &messages);
    void productNameChanged(const QString &productName);
    void canShowProductWindowChanged(const bool canShowProductWindow);
    void userChanged(int accessLevel, QString const& username, QString const& uuid, QString const& card_id);
    
    // CommunicationInterface
    void valuesChanged(const QObject * sender, const QHash<QString, QVariant> &values);
    void valueChanged(const QObject * sender, const QString &name, const QVariant &value);

    // ImageProducerInterface
    void newImage(QString cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image);

    // InspectionResultProducerInterface
    void finishedInspection(const std::shared_ptr<const InspectionResult> &result);

public slots:
    // PluginInterface
    void setRequestedMachineState(PluginInterface::MachineState state) { _requested_machine_state = state; emit requestedMachineStateChanged(state); }
    
    // CommunicationInterface
    void setValues(const QHash<QString, QVariant> &values);
    void setValue(const QString &name, const QVariant &value);
    
    void aboutToQuit();

private:
    void updatePluginStates();
    
    void registerMetaTypes();
    MainWindow *_mainWindow;
    AlarmDatabase *_db;
    
    QObjectList _available_plugins;

    void loadPlugin(QObject *object);
    QObjectList _loaded_plugins;

    bool _plugins_enabled;
    QList<QThread *> _plugin_threads;
    
    PluginInterface::MachineState _requested_machine_state;
    PluginInterface::MachineState _current_machine_state;
    PluginInterface::DiagState _current_diag_state;
    uint _current_access_level;
    QReadWriteLock _messages_lock;
    QList<PluginInterface::Message> _messages;
    QString _current_product_name;
    bool _can_show_product_window;
};

#endif // PLUGINSCONTROLLER_H
