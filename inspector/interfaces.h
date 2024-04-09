#ifndef INTERFACES_H
#define INTERFACES_H

#include <memory>

#include <QtCore>
#include <QtWidgets>
#include <QDebug>


namespace cv {
    class Mat;
}

class InspectionResult;
class Logic;
class OptionPanel;


Q_DECLARE_METATYPE(std::shared_ptr<const InspectionResult>)
Q_DECLARE_METATYPE(std::shared_ptr<Logic>)

const char * const kRequiredAccessLevel = "requiredAccessLevel";
const int kAccessLevelGuest     = 0;
const int kAccessLevelUser      = 2;
const int kAccessLevelService   = 3;
const int kAccessLevelSysOp     = 8;
const int kAccessLevelAdmin     = 9;
const int kAccessLevelBertram   = 10;
const int kAccessLevelNA        = INT_MAX;


class PluginInterface
{
public:
    virtual ~PluginInterface() {}

    typedef enum {
        Terminate,
        Initializing,
        EmergencyStop,
        Error,
        Stopping,
        Off,
        Starting,
        Setup,
        Production
    } MachineState;

    typedef enum {
        Alarm,
        WarningHigh,
        WarningLow,
        OK
    } DiagState;
    
    struct Message {
        uint code;
        QDateTime timestamp;
        QtMsgType type;
        QString message;
        QString plugin;
        
        Message() {}
        Message(uint init_code) { code = init_code; }
        Message(uint init_code, QString init_message, QtMsgType init_type, QDateTime init_timestamp = QDateTime::currentDateTime(), QString init_plugin = QString()) {
            code = init_code;
            message = init_message;
            type = init_type;
            timestamp = init_timestamp;
            plugin = init_plugin;
        }
        
        inline bool operator==(const Message &other) {
            return code == other.code
                && type == other.type
                && message == other.message
                && plugin == other.plugin;
        }
    };
    
    virtual const QString identifier() const = 0;
    virtual const QString name() const = 0;
    
    virtual const MachineState machineState() const = 0;
    virtual void updateMessages() {}
    const QList<Message> messages() { updateMessages(); QReadLocker locker(&_messages_lock);  return QList<Message>(_messages); }
    
    virtual const QString currentProductName() const { return QString(); };
    virtual bool canShowProductWindow() const { return false; }
    
    virtual QThread::Priority priority() const { return QThread::InheritPriority; }
    virtual void moveToThread(QThread *thread) {}
    
    const QVariant getPreference(const QString & preference) { QSettings settings; return settings.value("Plugins/" + identifier() + "/" + preference); }
    void setPreference(const QString & preference, const QVariant & value) { QSettings settings; settings.setValue("Plugins/" + identifier() + "/" + preference, value); }

protected: // signals
    // message handling
    void clearMessages() {
        QWriteLocker locker(&_messages_lock);
        _messages.clear();
    }
    
    void updateMessages(const QList<Message> &messages) {
        // update message ids to avoid clashes with other plugins
        QList<Message> updated_messages;
        for (const Message &message : messages) {
            if (message.plugin.isEmpty()) {
                updated_messages << Message(message.code, message.message, message.type, message.timestamp, name());
            } else {
                updated_messages << message;
            }
        }
        
        QWriteLocker locker(&_messages_lock);
        QMutableListIterator<Message> i(_messages);
        while (i.hasNext()) {
            if (!updated_messages.contains(i.next())) {
                i.remove();
            }
        }
        for (const Message &m : updated_messages) {
            if (!_messages.contains(m)) {
                _messages << m;
            }
        }
    }
    
    void performReset();
    void changeMachineState(PluginInterface::MachineState state);
    
public: // slots
    virtual void initialize() { }
    virtual void uninitialize() { }
    
    virtual void reset() { }
    virtual void requestMachineState(const PluginInterface::MachineState state) { }
    virtual void currentMachineState(const PluginInterface::MachineState machineState, const PluginInterface::DiagState diagState) { }
    
    virtual void showProductWindow() { }

    virtual void currentUser(int access_level, QString const& username, QString const& uuid, QString const& card_id) {}
    
private:
    QReadWriteLock _messages_lock;
    QList<Message> _messages;

};
Q_DECLARE_INTERFACE(PluginInterface, "de.bertram-bildverarbeitung.PluginInterface/1.0")

inline QDebug operator<<(QDebug dbg, const PluginInterface::Message &m) {
    dbg.nospace() << "Message(" << m.timestamp << ", " << m.type << ", " << m.code << ", " << m.plugin << ", " << m.message << ")";
    return dbg.space();
}

inline bool operator==(const PluginInterface::Message &message1, const PluginInterface::Message &message2)
{
    return message1.code == message2.code && message1.plugin == message2.plugin;
}

inline uint qHash(const PluginInterface::Message &message, uint seed)
{
    return qHash(message.code, seed);
}

class MainWindowInterface
{
public:
    virtual ~MainWindowInterface() {}
    
    typedef enum {
        Application,
        Statistics,
        Messages,
        Diagnostics,
        Settings
    } WidgetType;
    
    virtual const WidgetType widgetType(const int idx) const { (void)idx; return Application; };
    virtual const QString title(const int idx) const = 0;
    virtual QWidget *mainWidget(const int idx) const = 0;
    virtual OptionPanel *optionPanel(const int idx) const { (void)idx; return nullptr; }
    virtual int preferredTabIndex(const int idx) const { (void)idx; return INT_MAX; }
    virtual int requiredWidgetAccessLevel(const int idx) const { (void)idx; return kAccessLevelGuest; }

    virtual int mainWidgetCount() const { return 1; }

    int currentAccessLevel() const { return _current_access_level; }
    QString currentUsername() const { return _current_username; }
    QString currentUuid() const { return _current_uuid; }
    QString currentCardId() const { return _current_card_id; }

    void setAccessLevel(QWidget *widget, int accessLevel, QString username, QString uuid, QString card_id) {
        _current_access_level = accessLevel;
        _current_username = username;
        _current_uuid = uuid;
        _current_card_id = card_id;

        if (widget->dynamicPropertyNames().contains(kRequiredAccessLevel)) {
            int requiredAccessLevel = widget->property(kRequiredAccessLevel).toInt();
            widget->setEnabled(accessLevel >= requiredAccessLevel);
        } else {
            // Recurse through all widget children
            for (QObject *child : widget->children()) {
                if (child->isWidgetType()) {
                    setAccessLevel(qobject_cast<QWidget*>(child), accessLevel, username, uuid, card_id);
                }
            }
        }
    }
    
protected: // signals
    
public: // slots
    
private:
    int _current_access_level = kAccessLevelGuest;
    QString _current_uuid;
    QString _current_username;
    QString _current_card_id;

};
Q_DECLARE_INTERFACE(MainWindowInterface, "de.bertram-bildverarbeitung.MainWindowInterface/1.0")


class CommunicationInterface
{

public:
    virtual ~CommunicationInterface() {}

protected: // signals
    virtual void valueChanged(const QString &name, const QVariant &value) = 0;
    virtual void valuesChanged(const QHash<QString, QVariant> &values) = 0;

public:
    virtual void setValue(const QString &name, const QVariant &value) = 0;
    virtual void setValues(const QHash<QString, QVariant> &values) = 0;

    // usable default implementations
    inline void defaultSetValue(const QString &name, const QVariant &value)
    {
        QHash<QString, QVariant> values;
        values[name] = value;
        setValues(values);
    }
    inline void defaultSetValues(const QHash<QString, QVariant> &values)
    {
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            setValue(it.key(), it.value());
        }
    }
};
Q_DECLARE_INTERFACE(CommunicationInterface, "de.bertram-bildverarbeitung.CommunicationInterface/1.0")


class ImageConsumerInterface
{
public:
    virtual ~ImageConsumerInterface() {}

public: // slots
    virtual void consumeImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image) = 0;
};
Q_DECLARE_INTERFACE(ImageConsumerInterface, "de.bertram-bildverarbeitung.ImageConsumerInterface/2.0")


class ImageProducerInterface
{
public:
    virtual ~ImageProducerInterface() {}
    
protected: // signals
    virtual void newImage(const QString &cameraId, unsigned long frameNumber, int frameStatus, const cv::Mat &image) = 0;
};
Q_DECLARE_INTERFACE(ImageProducerInterface, "de.bertram-bildverarbeitung.ImageProducerInterface/2.0")


class InspectionResultConsumerInterface
{
public:
    virtual ~InspectionResultConsumerInterface() {}

public: // slots
     virtual void consumeInspectionResult(const std::shared_ptr<const InspectionResult> &result) = 0;
};
Q_DECLARE_INTERFACE(InspectionResultConsumerInterface, "de.bertram-bildverarbeitung.InspectionResultConsumerInterface/1.0")


class InspectionResultProducerInterface
{
public:
    virtual ~InspectionResultProducerInterface() {}

protected: // signals
    virtual void finishedInspection(const std::shared_ptr<const InspectionResult> &result) = 0;
};
Q_DECLARE_INTERFACE(InspectionResultProducerInterface, "de.bertram-bildverarbeitung.InspectionResultProducerInterface/1.0")

#endif // INTERFACES_H
