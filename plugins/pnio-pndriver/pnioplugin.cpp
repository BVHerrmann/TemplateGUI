#include "pnioplugin.h"

#include "pniocontroller.h"
#include "pniodevice.h"
#include "pniowidget.h"

#include "logic.h"


PNIOPlugin::PNIOPlugin(QObject *parent) :
	Plugin(parent)
{
	_widget = new PNIOWidget(this);
	connect(this, &PNIOPlugin::setupWidget, _widget, &PNIOWidget::setupWidget);

	// start a timer on the main thread!
	_timer = new QTimer();
	//NOTE: Print statistic augeklammert
	//connect(_timer, &QTimer::timeout, this, &PNIOPlugin::printStatistics, Qt::DirectConnection);
	_timer->start(60000);
}

PNIOPlugin::~PNIOPlugin()
{
    _timer->stop();
    delete _timer;
}

const PluginInterface::MachineState PNIOPlugin::machineState() const
{
    if (_request_state == PluginInterface::Terminate) {
        return PluginInterface::Terminate;
    } else if (!_interface || !_interface->isInitialized()) {
        return PluginInterface::Initializing;
    } else if (_interface->hasError()) {
        return PluginInterface::Error;
    } else {
        return Production;
    }
}

void PNIOPlugin::updateMessages()
{
    if (_interface) {
        PluginInterface::updateMessages(_interface->alarmMessages());
    }
}

void PNIOPlugin::uninitialize()
{
    disconnectPNIO();
}

void PNIOPlugin::printStatistics()
{
    if (_interface) {
        _interface->printStatistics();
    }
}

void PNIOPlugin::connectPNIO()
{
    if (_interface)
        _interface->connectPNIO();
}

void PNIOPlugin::disconnectPNIO()
{
    if (_interface)
        _interface->disconnectPNIO();
}

void PNIOPlugin::currentMachineState(const PluginInterface::MachineState machineState, const PluginInterface::DiagState diagState)
{
    if (_interface)
        _interface->currentMachineState(machineState, diagState);
}

void PNIOPlugin::setValue(const QString &name, const QVariant &value)
{
	// check for special messages
	if (!_interface && name == "PNIO/configure") {
		QHash<QString, QVariant> config = value.toHash();

		// convert in and out config
		QVector<std::shared_ptr<PNIOInputValue>> in_config_tmp = config["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>();
		QVector<std::shared_ptr<PNIOOutputValue>> out_config_tmp = config["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>();
		QVector<std::shared_ptr<PNIORecord>> record_config_tmp = config["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>();
		auto in_config = std::vector<std::shared_ptr<PNIOInputValue>>(in_config_tmp.begin(), in_config_tmp.end());
		auto out_config = std::vector<std::shared_ptr<PNIOOutputValue>>(out_config_tmp.begin(), out_config_tmp.end());
		auto record_config = std::vector<std::shared_ptr<PNIORecord>>(record_config_tmp.begin(), record_config_tmp.end());

		//NOTE: Prüfung auf vorhadene HardwareConf und networkId? 
		// create interface
		if (config["type"] == "controller") {
			_interface = std::shared_ptr<PNIOController>(new PNIOController(in_config, out_config, record_config, config["config"].toByteArray(), config["network_interface_mac"].toByteArray()));
		} else if (config["type"] == "device") {
			//_interface = std::shared_ptr<PNIODevice>(new PNIODevice(in_config, out_config));
		}

		if (_interface) {
			// directly connect logic
			if (config.contains("logic") && config["logic"].canConvert<std::shared_ptr<Logic> >()) {
				std::shared_ptr<Logic> logic = config["logic"].value<std::shared_ptr<Logic> >();
				if (logic) {
					_interface->setLogic(logic);
					logic->setInterface(_interface);
				}
			}

			// needs to use connection, as widget stuff has to be done on main thread
			if (in_config.size() || out_config.size())
				emit setupWidget(_interface.get());

			// wire new interface
			connect(_interface.get(), &PNIOInterface::valueChanged, this, &PNIOPlugin::valueChanged);

			QTimer::singleShot(1000, _interface.get(), &PNIOInterface::connectPNIO);
		}
		else {
			qWarning() << config << "does not contain a valid interface type!";
		}

    } else if (_interface && name == "PNIO/setup") {
		QHash<QString, QVariant> config = value.toHash();

		// convert in and out config
		QVector<std::shared_ptr<PNIOInputValue>> in_config_tmp = config["in_config"].value<QVector<std::shared_ptr<PNIOInputValue>>>();
		QVector<std::shared_ptr<PNIOOutputValue>> out_config_tmp = config["out_config"].value<QVector<std::shared_ptr<PNIOOutputValue>>>();
		QVector<std::shared_ptr<PNIORecord>> record_config_tmp = config["record_config"].value<QVector<std::shared_ptr<PNIORecord>>>();
		auto in_config = std::vector<std::shared_ptr<PNIOInputValue>>(in_config_tmp.begin(), in_config_tmp.end());
		auto out_config = std::vector<std::shared_ptr<PNIOOutputValue>>(out_config_tmp.begin(), out_config_tmp.end());
		auto record_config = std::vector<std::shared_ptr<PNIORecord>>(record_config_tmp.begin(), record_config_tmp.end());

        _interface->updateConfig(in_config, out_config, record_config);

		// needs to use connection, as widget stuff has to be done on main thread
		if (in_config.size() || out_config.size())
			emit setupWidget(_interface.get());
    }
}
