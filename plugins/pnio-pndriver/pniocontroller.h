#ifndef PNIOCONTROLLER_H
#define PNIOCONTROLLER_H

#include <QtCore>

#include "pniointerface.h"

#include <pniousrx.h>
#include <servusrx.h>
#include "pndriver_product_info.h"

Q_DECLARE_METATYPE(PNIO_MODE_TYPE)

class PNIOController : public PNIOInterface
{
    Q_OBJECT
public:
	explicit PNIOController(
		const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, 
		const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config,
		const std::vector<std::shared_ptr<PNIORecord>> &record_config,
		QByteArray hardwareConfiguration, 
		QByteArray _macAddress);

	void updateConfig(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config,
		const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config,
		const std::vector<std::shared_ptr<PNIORecord>> &record_config) override;

    bool isInitialized() const override { return _is_initialized; }

signals:

public slots:
	void connectPNIO() override;
	void disconnectPNIO() override;

	// callbacks
	void setAlarm(const PNIO_UINT32 handle, const PNIO_CTRL_ALARM_DATA alarm);
	void setConfig(const PNIO_UINT32 handle, const std::vector<PNIO_MODULE> config);
	void setMode(const PNIO_UINT32 handle, const PNIO_MODE_TYPE mode);
	
	void readReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_READ_CONF readConf);
	void writeReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_WRITE_CONF writeConf);

private:
	// private functions for connecting and disconnecting
	PNIO_UINT32 initialize() override;
	PNIO_UINT32 uninitialize() override;
    bool _is_initialized = false;

	PNIO_UINT32 changeAndWaitForMode(const PNIO_MODE_TYPE mode);
	PNIO_MODE_TYPE _mode;

	QMutex cb_config_mutex;
	QWaitCondition cb_config_cond;

	QMutex cb_mode_mutex;
	QWaitCondition cb_mode_cond;

	// triggers async reading and writing of data to the controller.
	void readDataInternal() override;
	std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> writeData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data) override;
	void setRecordFunctions(void);

	//Records
	bool writeDataRequest(const PNIO_UINT32 slot, const PNIO_UINT32 record_index, const PNIO_UINT32 req_ref, const std::vector<std::byte> &data);
	bool readDataRequest(const PNIO_UINT32 slot, const PNIO_UINT32 record_index, const PNIO_UINT32 req_ref);

	QByteArray _hardwareConfiguration;
	QByteArray _macAddress;
};

class PNIOControllerCallbackHelper : public QObject
{
    Q_OBJECT
public:
    explicit PNIOControllerCallbackHelper(QObject *parent = 0);

signals:
	void alarmOccured(const PNIO_UINT32 handle, const PNIO_CTRL_ALARM_DATA alarm);
	void configChanged(const PNIO_UINT32 handle, const std::vector<PNIO_MODULE> config);
	void modeChanged(const PNIO_UINT32 handle, const PNIO_MODE_TYPE mode);
	
	void readReqReady(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_READ_CONF readConf);
	void writeReqReady(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_WRITE_CONF writeConf);

public slots:
	void setAlarm(const PNIO_UINT32 handle, const PNIO_CTRL_ALARM_DATA alarm);
	void setConfig(const PNIO_UINT32 handle, const std::vector<PNIO_MODULE> config);
	void setMode(const PNIO_UINT32 handle, const PNIO_MODE_TYPE mode);
	
	void readReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_READ_CONF readConf);
	void writeReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_WRITE_CONF writeConf);
};

#endif // PNIOCONTROLLER_H
