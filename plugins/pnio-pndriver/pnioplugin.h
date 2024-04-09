#ifndef PNIOPLUGIN_H
#define PNIOPLUGIN_H

#include <plugin.h>
#include <interfaces.h>
//#include <logic.h>

#include <QtCore>

#include "pniointerface.h"
#include "pniowidget.h"

class PNIOPlugin : public Plugin, PluginInterface, MainWindowInterface, CommunicationInterface
{
	Q_OBJECT
		Q_PLUGIN_METADATA(IID "de.bertram-bildverarbeitung.PNIOPlugin")
		Q_INTERFACES(PluginInterface)
		Q_INTERFACES(MainWindowInterface)
		Q_INTERFACES(CommunicationInterface)

public:
	enum Type {
		INT8,
		UINT8,
		INT16,
		UINT16,
		INT32,
		UINT32,
		INT64,
		UINT64,
		FLOAT32,
		RAW8,
		RAW16,
		RAW32,
		RAW64
	};
    
	explicit PNIOPlugin(QObject *parent = nullptr);
	virtual ~PNIOPlugin();

	// PluginInterface
	const QString identifier() const override { return "PNDriver"; }
	const QString name() const override { return tr("ProfiNET (PNDriver)"); }
    const MachineState machineState() const override;
    void updateMessages();
	QThread::Priority priority() const override { return QThread::IdlePriority; }

	// MainWindowInterface
	const WidgetType widgetType(const int idx) const override { Q_UNUSED(idx); return Diagnostics; }
	const QString title(const int idx) const override { Q_UNUSED(idx); return name(); }
	QWidget * mainWidget(const int idx) const override { Q_UNUSED(idx); return _widget; }
	int preferredTabIndex(const int idx) const override { Q_UNUSED(idx); return INT_MAX; }
	int requiredWidgetAccessLevel(const int idx) const override { Q_UNUSED(idx); return kAccessLevelSysOp; }

    //QHash<QString, QVariant> readData();
	void writeValues(QHash<QString, QVariant> values);

signals:
	// CommunicationInterface
	void valuesChanged(const QHash<QString, QVariant> &values) override;
	void valueChanged(const QString &name, const QVariant &value) override;

	// Helper
	void setupWidget(PNIOInterface *pnio_interface);

public slots:
	// PluginInterface
	void uninitialize() override;
    void requestMachineState(const PluginInterface::MachineState state) override { _request_state = state; }
    void currentMachineState(const PluginInterface::MachineState machineState, const PluginInterface::DiagState diagState);

	// CommunicationInterface
	void setValue(const QString &name, const QVariant &value) override;
	void setValues(const QHash<QString, QVariant> &values) override { defaultSetValues(values); }

	void printStatistics();

	void connectPNIO();
	void disconnectPNIO();

private:
    PluginInterface::MachineState _request_state = PluginInterface::MachineState::Production;

	PNIOWidget *_widget;

	std::shared_ptr<PNIOInterface> _interface;

	QTimer *_timer;
};

#endif // PNIOPLUGIN_H
