#include "pniocontroller.h"

// define callbacks
void callback_for_ds_read_conf(PNIO_CBE_PRM * pCbfPrm);
void callback_for_ds_write_conf(PNIO_CBE_PRM * pCbfPrm);
void callback_for_mode_change_indication(PNIO_CBE_PRM * pCbfPrm);
void callback_for_device_activation(PNIO_CBE_PRM * pCbfPrm);
void callback_for_alarm_indication(PNIO_CBE_PRM * pCbfPrm);
void callback_for_stop_request(PNIO_CBE_PRM * pCbfPrm);
void callback_for_led_flash(PNIO_CBE_PRM * pCbfPrm);
void callback_for_diagnostics(PNIO_CBE_PRM * pCbfPrm);


// Some callback-flags for the callbacks we have to wait for in order to initialize the device properly
static PNIOControllerCallbackHelper *controller_callback_helper = 0;

#define CP_INDEX 1  // Unique identification for the communication module (module index in the component configuration)


PNIOController::PNIOController(
	const std::vector<std::shared_ptr<PNIOInputValue>> &in_config,
	const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config,
	const std::vector<std::shared_ptr<PNIORecord>> &record_config,
	QByteArray hardwareConfiguration,
	QByteArray macAddress) :
	PNIOInterface(in_config, out_config, record_config)
{
	//register HW
	_hardwareConfiguration = hardwareConfiguration;
	_macAddress = macAddress;

	// register types used in signals and slots
	qRegisterMetaType<PNIO_MODE_TYPE>("PNIO_MODE_TYPE");

    // controller is offline at launch
    _mode = PNIO_MODE_OFFLINE;

	// make sure there is a callback helper
	if (controller_callback_helper == 0)
		controller_callback_helper = new PNIOControllerCallbackHelper();

	// listen to callbacks. Qt::DirectConnection is important to make sure callbacks are called from a different thread
	connect(controller_callback_helper, &PNIOControllerCallbackHelper::alarmOccured, this, &PNIOController::setAlarm, Qt::DirectConnection);
	connect(controller_callback_helper, &PNIOControllerCallbackHelper::configChanged, this, &PNIOController::setConfig, Qt::DirectConnection);
	connect(controller_callback_helper, &PNIOControllerCallbackHelper::modeChanged, this, &PNIOController::setMode, Qt::DirectConnection);

	connect(controller_callback_helper, &PNIOControllerCallbackHelper::readReqReady, this, &PNIOController::readReqDone, Qt::DirectConnection);
	connect(controller_callback_helper, &PNIOControllerCallbackHelper::writeReqReady, this, &PNIOController::writeReqDone, Qt::DirectConnection);

    bool _is_initialized = false;
}

PNIO_UINT32 PNIOController::initialize()
{
    qDebug() << "Initialize Controller";

    PNIO_UINT32 errorCode = PNIO_OK;

	// Init CP
	errorCode = SERV_CP_init(0);
	if (errorCode != PNIO_OK) {
		qCritical() << tr("Error initializing the CP!") << errorDescription(errorCode);
		return errorCode;
	}

	// get Network Adapter
	PNIO_UINT8 NrOfCp;
	PNIO_CP_ID_PTR_TYPE CpList;
	int PndMaxNrOfCp = 50;
	CpList = (PNIO_CP_ID_PTR_TYPE)malloc(sizeof(PNIO_CP_ID_TYPE) * PndMaxNrOfCp);

	PNIO_CP_ID_TYPE CpID;

	errorCode = SERV_CP_get_network_adapters(CpList, &NrOfCp);
	if (errorCode != PNIO_OK) {
		qCritical() << tr("Error no network adapters detected!") << errorDescription(errorCode);
		return errorCode;
	}


	for (int i = 0; i < sizeof(CpList); i++) {
		if (QByteArray((const char*)CpList[i].CpMacAddr, PNIO_MAC_ADDR_SIZE) == _macAddress) {
			memcpy(&CpID, &CpList[i], sizeof(PNIO_CP_ID_TYPE));
			errorCode = PNIO_OK;
			break;
		}
		else {
			errorCode = PNIO_ERR_INVALID_CONFIG;
		}
	}
	free(CpList);

	if (errorCode != PNIO_OK) {
		qCritical() << tr("Error wrong MAC Address was passed!") << errorDescription(errorCode);
		return errorCode;
	}

	// start CP
	const char* config = _hardwareConfiguration.constData();

	char* remData = nullptr;
	PNIO_UINT32 remaFileSize = 0;

	PNIO_SYSTEM_DESCR systemDescription;
	strcpy((char*)(&systemDescription.Vendor), PND_VENDOR);
	strcpy((char*)(&systemDescription.ProductFamily), PND_PRODUCT_FAMILY);
	strcpy((char*)(&systemDescription.IM_DeviceType), PND_IM_DEVICE_TYPE);
	strcpy((char*)(&systemDescription.IM_OrderId), PND_MLFB);
	systemDescription.IM_HwRevision = PND_HW_REVISION;
	systemDescription.IM_SwVersion.revision_prefix = PND_VERSION_REVISION_PREFIX;
	systemDescription.IM_SwVersion.functional_enhancement = PND_VERSION_FUNCTIONAL_ENHANCEMENT;
	systemDescription.IM_SwVersion.bug_fix = PND_VERSION_BUG_FIX;
	systemDescription.IM_SwVersion.internal_change = PND_VERSION_INTERNAL_CHANGE;
	strcpy((char*)(&systemDescription.ProductSerialNr), PND_PRODUCT_SERIAL_NUMBER);

	errorCode = SERV_CP_startup(&CpID, 1, (PNIO_UINT8*)config, _hardwareConfiguration.size(), (PNIO_UINT8*)remData, remaFileSize,  &systemDescription);

	if (errorCode != PNIO_OK) {
		qCritical() << tr("Error initializing the CP!") << errorDescription(errorCode);
		return errorCode;
	}


    // open controller
    errorCode = PNIO_controller_open(CP_INDEX, PNIO_CEP_MODE_CTRL, callback_for_ds_read_conf, callback_for_ds_write_conf, callback_for_alarm_indication, &_handle);

	if (errorCode != PNIO_OK) {
        qCritical() << tr("Error initializing the controller!") << errorDescription(errorCode);
        return errorCode;
    }

    // register mode change callbacks
    errorCode = PNIO_register_cbf(_handle, PNIO_CBE_MODE_IND, callback_for_mode_change_indication);
    if (errorCode != PNIO_OK) {
        qCritical() << tr("Error registering PNIO_register_cbf PNIO_CBE_MODE_IND!") << errorDescription(errorCode);
        PNIO_close(_handle);
        return errorCode;
    }

    // register callback for device activation
    errorCode = PNIO_register_cbf(_handle, PNIO_CBE_DEV_ACT_CONF, callback_for_device_activation);
    if (errorCode != PNIO_OK) {
        qCritical() << tr("Error registering PNIO_register_cbf PNIO_CBE_DEV_ACT_CONF!") << errorDescription(errorCode);
        PNIO_close(_handle);
        return errorCode;
    }

    // register callback for remote stop request
    errorCode = PNIO_register_cbf(_handle, PNIO_CBE_CP_STOP_REQ, callback_for_stop_request);
    if (errorCode != PNIO_OK) {
        qCritical() << tr("Error registering PNIO_register_cbf PNIO_CBE_CP_STOP_REQ!") << errorDescription(errorCode);
        PNIO_close(_handle);
        return errorCode;
    }

	// register callback for diagnostics
    errorCode = PNIO_register_cbf(_handle, PNIO_CBE_CTRL_DIAG_CONF, callback_for_diagnostics);
    if (errorCode != PNIO_OK) {
        qCritical() << tr("Error registering PNIO_register_cbf PNIO_CBE_CTRL_DIAG_CONF!") << errorDescription(errorCode);
        PNIO_close(_handle);
        return errorCode;
    }

	// get config
	cb_config_mutex.lock();

    PNIO_CTRL_DIAG pConfigRequest = {PNIO_CTRL_DIAG_CONFIG_SUBMODULE_LIST, {{0, 0}}, 0, 0};
	errorCode = PNIO_ctrl_diag_req(_handle, &pConfigRequest);

	if (errorCode != PNIO_OK) {
		// unlock mutex
        cb_config_mutex.unlock();

        qCritical() << tr("Error calling PNIO_ctrl_diag_req for PNIO_CTRL_DIAG_CONFIG_SUBMODULE_LIST!") << errorDescription(errorCode);
        if (errorCode != PNIO_ERR_INVALID_CONFIG) {
            PNIO_close(_handle);
            return errorCode;
        } else {
            // errorCode = PNIO_OK;
        }
    } else {
		qDebug() << "PNIO_ctrl_diag_req sent!";
	}

	// wait (10s) for callback
    if (!cb_config_cond.wait(&cb_config_mutex, 10000))
        qCritical() << tr("Missing config confirmation!");
    cb_config_mutex.unlock();

	/*
	PNIO_CTRL_DIAG pDiagReq2 = {PNIO_CTRL_DIAG_DEVICE_STATE};
	errorCode = PNIO_ctrl_diag_req(_handle, &pDiagReq2);
	if (errorCode != PNIO_OK) {
        qCritical() << tr("Error calling PNIO_ctrl_diag_req for PNIO_CTRL_DIAG_DEVICE_STATE!") << errorDescription(errorCode);
        errorCode = PNIO_OK;
    } else {
		qDebug() << "PNIO_ctrl_diag_req sent!";
	}

	PNIO_CTRL_DIAG pDiagReq3 = {PNIO_CTRL_DIAG_CONFIG_IOROUTER_PRESENT};
	errorCode = PNIO_ctrl_diag_req(_handle, &pDiagReq3);
	if (errorCode != PNIO_OK) {
        qCritical() << tr("Error calling PNIO_ctrl_diag_req for PNIO_CTRL_DIAG_CONFIG_IOROUTER_PRESENT!") << errorDescription(errorCode);
        errorCode = PNIO_OK;
    } else {
		qDebug() << "PNIO_ctrl_diag_req sent!";
	}

	PNIO_CTRL_DIAG pDiagReq4 = {PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE_LIST};
	errorCode = PNIO_ctrl_diag_req(_handle, &pDiagReq4);
	if (errorCode != PNIO_OK) {
        qCritical() << tr("Error calling PNIO_ctrl_diag_req for PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE_LIST!") << errorDescription(errorCode);
        errorCode = PNIO_OK;
    } else {
		qDebug() << "PNIO_ctrl_diag_req sent!";
	}

	PNIO_CTRL_DIAG pDiagReq5 = {PNIO_CTRL_DIAG_CONFIG_NAME_ADDR_INFO};
	errorCode = PNIO_ctrl_diag_req(_handle, &pDiagReq5);
	if (errorCode != PNIO_OK) {
        qCritical() << tr("Error calling PNIO_ctrl_diag_req for PNIO_CTRL_DIAG_CONFIG_NAME_ADDR_INFO!") << errorDescription(errorCode);
        errorCode = PNIO_OK;
    } else {
		qDebug() << "PNIO_ctrl_diag_req sent!";
	}
	*/

	// register IRT callbacks
	errorCode = registerIRT();

    return errorCode;
}

PNIO_UINT32 PNIOController::uninitialize()
{
    PNIO_UINT32 errorCode = PNIO_OK;

    errorCode = changeAndWaitForMode(PNIO_MODE_OFFLINE);
    if (errorCode != PNIO_OK)
        return errorCode;

    errorCode = PNIO_close(_handle);
    if (errorCode != PNIO_OK) {
        qWarning() << tr("Error closing the controller!") << errorDescription(errorCode);
        return errorCode;
    }

	errorCode = SERV_CP_shutdown();
	if (errorCode != PNIO_OK) {
		qWarning() << tr("Error closing the CP!") << errorDescription(errorCode);
		return errorCode;
	}

	errorCode = SERV_CP_undo_init();
	if (errorCode != PNIO_OK) {
		qWarning() << tr("Error undo initializing of CP!") << errorDescription(errorCode);
		return errorCode;
	}

	// stop CP

    return errorCode;
}

void PNIOController::connectPNIO()
{
	// initialize controller
    PNIO_UINT32 errorCode = initialize();
    if (errorCode == PNIO_OK) {
        _is_initialized = true;
    } else {
        qCritical() << "Failed to initialize PNIO Controller with Error:" << errorDescription(errorCode);
		return;
    }

    errorCode = changeAndWaitForMode(PNIO_MODE_OPERATE);
    if (errorCode != PNIO_OK)
        return;
}

void PNIOController::updateConfig(const std::vector<std::shared_ptr<PNIOInputValue>>& in_config, const std::vector<std::shared_ptr<PNIOOutputValue>>& out_config, const std::vector<std::shared_ptr<PNIORecord>>& record_config)
{
    PNIOInterface::updateConfig(in_config, out_config, record_config);

    for (const std::shared_ptr<PNIOWriteRecord>& config : _write_record_config) {
        config->set_write_req_func(std::bind(&PNIOController::writeDataRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    for (const std::shared_ptr<PNIOReadRecord>& config : _read_record_config) {
        config->set_read_req_func(std::bind(&PNIOController::readDataRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
}

void PNIOController::disconnectPNIO()
{
    stopTimer();

    PNIO_UINT32 errorCode = uninitialize();
    if (errorCode == PNIO_OK) {
        _is_initialized = false;
    } else {
        qCritical() << "Failed to uninitialize PNIO Controller with Error:" << errorDescription(errorCode);
        return;
    }
}

void PNIOController::readReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_READ_CONF readConf)
{
	//TODO: überarbeiten
	if (handle == _handle) {
		for (const std::shared_ptr<PNIOReadRecord> &record : _read_record_config) {
			if (_in_slotToHardwareId[record->slot()] == readConf.pAddr->u.Addr
				&& record->recordIndex() == readConf.RecordIndex
				&& record->reqRef() == readConf.ReqRef) {

				std::vector<std::byte> data(readConf.Length);
				memcpy(data.data(), readConf.pBuffer, readConf.Length);
				record->update(data, readConf.Err.ErrCode == PNIO_OK);
			}
		}
	}
	else {
		qWarning() << "Invalid Handle";
	}
}

void PNIOController::writeReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_WRITE_CONF writeConf)
{
	if (handle == _handle) {
		for (const std::shared_ptr<PNIOWriteRecord> &record : _write_record_config) {
			if (_in_slotToHardwareId[record->slot()] == writeConf.pAddr->u.Addr
				&& record->recordIndex() == writeConf.RecordIndex
				&& record->reqRef() == writeConf.ReqRef) {

				record->update(writeConf.Err.ErrCode == PNIO_OK);
			}
		}
	}
	else {
		qWarning() << "Invalid Handle";
	}
}

void PNIOController::setAlarm(const PNIO_UINT32 handle, const PNIO_CTRL_ALARM_DATA alarm)
{
    if (handle == _handle) {
        if (alarm.AlarmType == PNIO_ALARM_DEV_RETURN) {
            startTimer();
        }

        std::pair<PNIO_UINT16, PNIO_UINT16> device_slot = std::make_pair(alarm.DeviceNum, alarm.SlotNum);
        auto &error_mask = _module_errors[device_slot];

        switch (alarm.AlarmType) {
        case PNIO_ALARM_DEV_FAILURE:
            error_mask = error_mask | connectionLost;
            emit valueChanged("PNIO/controller_disconnected", QVariant());
            break;
        case PNIO_ALARM_DEV_RETURN:
            error_mask = error_mask & ~connectionLost;
            emit valueChanged("PNIO/controller_connected", QVariant());
            break;
        case PNIO_ALARM_PULL:
            error_mask = error_mask | moduleUnplugged;
            break;
        case PNIO_ALARM_PLUG:
            error_mask = error_mask & ~moduleUnplugged;
            break;
        default:
            qDebug() << "Unknown alarm type!";
            break;
        }
    }
}

void PNIOController::setConfig(const PNIO_UINT32 handle, const std::vector<PNIO_MODULE> config)
{
	if (handle == _handle) {
		// set new config
		_config = config;

		// initialize buffers
		prepareBuffer();

		// wake all waiting threads
        cb_config_mutex.lock();
        cb_config_cond.wakeAll();
        cb_config_mutex.unlock();

		// notify listening plugins about config
		QList<QVariant> c;
		foreach (PNIO_MODULE module, config) {
			QHash<QString, QVariant> m;
			m["slot"] = module.slot;
			m["subslot"] = module.subslot;
			m["data_length"] = module.data_length;
			m["io_type"] = module.io_type;
			m["hw_id"] = module.hw_id;

			c << m;

			if (module.io_type == PNIO_IO_IN) {
				_in_slotToHardwareId[module.slot] = module.hw_id;
			}
		}
		emit valueChanged("PNIOControllerConfig", c);
	}
}

void PNIOController::setMode(const PNIO_UINT32 handle, const PNIO_MODE_TYPE mode)
{
    if (handle == _handle) {
        _mode = mode;

        // wake all waiting threads
        cb_mode_mutex.lock();
        cb_mode_cond.wakeAll();
        cb_mode_mutex.unlock();
    }
}

PNIO_UINT32 PNIOController::changeAndWaitForMode(const PNIO_MODE_TYPE mode)
{
    PNIO_UINT32 errorCode = PNIO_OK;

    cb_mode_mutex.lock();

    /*setting  mode asynchronously                                 */
    errorCode = PNIO_set_mode(_handle, mode);

    if (errorCode != PNIO_OK) {
        // unlock mutex
        cb_mode_mutex.unlock();

        qCritical() << tr("Error in PNIO_set_mode!") << errorDescription(errorCode);
        return errorCode;
    }

    // wait (10s) for callback
    if (!cb_mode_cond.wait(&cb_mode_mutex, 10000))
        qCritical() << tr("Missing mode change confirmation!");
    cb_mode_mutex.unlock();

    return errorCode;
}

void PNIOController::readDataInternal()
{
	std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> data;
	std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> state;

	for (const PNIO_MODULE &module : _config) {
		if (module.io_type == PNIO_IO_IN) {
			std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = std::make_pair(module.slot, module.subslot);

			PNIO_UINT32 dwBytesReaded = 0;
			std::vector<std::byte> deviceInputData(module.data_length, std::byte('\0'));

			PNIO_IOXS localState = PNIO_S_GOOD;
			PNIO_IOXS remoteState = PNIO_S_BAD;

			PNIO_ADDR deviceInputAddress;
			deviceInputAddress.IODataType = PNIO_IO_IN;
			deviceInputAddress.AddrType = PNIO_ADDR_LOG;
			deviceInputAddress.u.Addr = module.slot;

			// read
			PNIO_UINT32 errorCode = PNIO_data_read(_handle, &deviceInputAddress, module.data_length, &dwBytesReaded, (PNIO_UINT8 *)deviceInputData.data(), localState, &remoteState);

			// check and notify about state change
			state[slot_subslot] = (remoteState == PNIO_S_GOOD && errorCode == PNIO_OK) ? PNIO_S_GOOD : PNIO_S_BAD;

            auto &module_error = _module_errors[std::make_pair(module.station_number, module.hardware_slot)];
            if (state[slot_subslot] == PNIO_S_BAD) {
                module_error = module_error | moduleReadErr;
            } else {
                module_error = module_error & ~moduleReadErr;
            }

			// store input data
			if (errorCode == PNIO_OK)
				data[slot_subslot] = deviceInputData;
		}
	}

	// forward
	processData(data, state);
}

std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> PNIOController::writeData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data)
{
	std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> out_states;

	for (auto &it : data) {
		// states
		PNIO_IOXS localState = PNIO_S_GOOD;
		PNIO_IOXS remoteState = PNIO_S_BAD;

		// address
		PNIO_ADDR deviceoutputAddress;
		deviceoutputAddress.IODataType = PNIO_IO_OUT;
		deviceoutputAddress.AddrType = PNIO_ADDR_LOG;
		deviceoutputAddress.u.Addr = it.first.first;

		// data
		PNIO_UINT32 out_size = it.second.size();
		PNIO_UINT8 *out_data = const_cast<PNIO_UINT8 *>(reinterpret_cast<const PNIO_UINT8 *>(it.second.data()));

		// write
		PNIO_UINT32 errorCode = PNIO_data_write(_handle, &deviceoutputAddress, out_size, out_data, localState, &remoteState);

		// check and notify about state change
		out_states[it.first] = (remoteState == PNIO_S_GOOD && errorCode == PNIO_OK) ? PNIO_S_GOOD : PNIO_S_BAD;

        for (PNIO_MODULE &module : _config) {
            if (module.slot == it.first.first && (module.io_type == PNIO_IO_OUT || module.io_type == PNIO_IO_IN_OUT)) {
                auto &module_error = _module_errors[std::make_pair(module.station_number, module.hardware_slot)];
                if (out_states[it.first] == PNIO_S_BAD) {
                    module_error = module_error | moduleWriteErr;
                } else {
                    module_error = module_error & ~moduleWriteErr;
                }
            }
        }

		// store output data
		_out_data[it.first] = it.second;
	}

	return out_states;
}

bool PNIOController::writeDataRequest(const PNIO_UINT32 slot, const PNIO_UINT32 record_index, const PNIO_UINT32 req_ref, const std::vector<std::byte> &data)
{
	//Address
	PNIO_ADDR Addr;
	Addr.IODataType = PNIO_IO_OUT;
	Addr.AddrType = PNIO_ADDR_LOG;
	Addr.u.Addr = _in_slotToHardwareId.at(slot);

	// data
	PNIO_UINT32 out_size = data.size();
	PNIO_UINT8 *out_data = (PNIO_UINT8 *)data.data();

	//write Request
	PNIO_UINT32 errorCode = PNIO_OK;
	errorCode = PNIO_rec_write_req(_handle, &Addr, record_index , req_ref, out_size, out_data);
	if (errorCode != PNIO_OK) {
		qCritical() << tr("Error write request!") << errorDescription(errorCode);
	}

	return errorCode == PNIO_OK;
}

bool PNIOController::readDataRequest(const PNIO_UINT32 slot, const PNIO_UINT32 record_index, const PNIO_UINT32 req_ref)
{
	//Address
	PNIO_ADDR Addr;
	Addr.IODataType = PNIO_IO_OUT;
	Addr.AddrType = PNIO_ADDR_LOG;
	Addr.u.Addr = _in_slotToHardwareId.at(slot);

	//read request
	PNIO_UINT32 errorCode = PNIO_OK;

	errorCode = PNIO_rec_read_req(_handle, &Addr, record_index, req_ref, 32768); // Max Byte that can be read 32768
	if (errorCode != PNIO_OK) {
		qCritical() << tr("Error read request!") << errorDescription(errorCode);
	}

	return errorCode == PNIO_OK;
}

void PNIOController::setRecordFunctions(void)
{
	for (const std::shared_ptr<PNIOWriteRecord>& config : _write_record_config) {
		config->set_write_req_func(std::bind(&PNIOController::writeDataRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}

	for (const std::shared_ptr<PNIOReadRecord>& config : _read_record_config) {
		config->set_read_req_func(std::bind(&PNIOController::readDataRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}



/*----------------------------------------------------------------------------------------------------*/
/*    CALLBACK HELPER                                                                                 */
/*----------------------------------------------------------------------------------------------------*/

PNIOControllerCallbackHelper::PNIOControllerCallbackHelper(QObject *parent) :
    QObject(parent)
{

}

void PNIOControllerCallbackHelper::readReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_READ_CONF readConf)
{
	emit readReqReady(handle, readConf);
}

void PNIOControllerCallbackHelper::writeReqDone(const PNIO_UINT32 handle, const PNIO_CBE_PRM_REC_WRITE_CONF writeConf)
{
	emit writeReqReady(handle, writeConf);
}


void PNIOControllerCallbackHelper::setAlarm(const PNIO_UINT32 handle, const PNIO_CTRL_ALARM_DATA alarm)
{
	emit alarmOccured(handle, alarm);
}

void PNIOControllerCallbackHelper::setConfig(const PNIO_UINT32 handle, const std::vector<PNIO_MODULE> config)
{
	emit configChanged(handle, config);
}

void PNIOControllerCallbackHelper::setMode(const PNIO_UINT32 handle, const PNIO_MODE_TYPE mode)
{
	emit modeChanged(handle, mode);
}

/*----------------------------------------------------------------------------------------------------*/
/*    CALLBACKS                                                                                       */
/*----------------------------------------------------------------------------------------------------*/

void callback_for_ds_read_conf(PNIO_CBE_PRM * pCbfPrm)
{
	/**************************************************************/
	/* Attention :                                                */
	/* this is a callback and must be returned as soon as possible */
	/* don't use any endless or time consuming functions          */
	/* e.g. exit() would be fatal                                 */
	/* defer all time consuming functionality to other threads    */
	/**************************************************************/

	if (pCbfPrm->CbeType == PNIO_CBE_REC_READ_CONF) {
		if (pCbfPrm->u.RecReadConf.Err.ErrCode != 0)
		{
			qDebug() << ("ErrCode    : 0x%x\n", pCbfPrm->u.RecReadConf.Err.ErrCode);
			qDebug() << ("ErrDecode  : 0x%x\n", pCbfPrm->u.RecReadConf.Err.ErrDecode);
			qDebug() << ("ErrCode1   : 0x%x\n", pCbfPrm->u.RecReadConf.Err.ErrCode1);
			qDebug() << ("ErrCode2   : 0x%x\n", pCbfPrm->u.RecReadConf.Err.ErrCode2);
			qDebug() << ("AddValue1  : 0x%x\n", pCbfPrm->u.RecReadConf.Err.AddValue1);
			qDebug() << ("AddValue2  : 0x%x\n", pCbfPrm->u.RecReadConf.Err.AddValue2);
		}

		if (controller_callback_helper != 0)
			controller_callback_helper->readReqDone(pCbfPrm->Handle, pCbfPrm->u.RecReadConf);
	}
}

void callback_for_ds_write_conf(PNIO_CBE_PRM * pCbfPrm)
{
	/**************************************************************/
	/* Attention :                                                */
	/* this is a callback and must be returned as soon as possible*/
	/* don't use any endless or time consuming functions          */
	/* e.g. exit() would be fatal                                 */
	/* defer all time consuming functionality to other threads    */
	/**************************************************************/

	if (pCbfPrm->CbeType == PNIO_CBE_REC_WRITE_CONF) {
		if (pCbfPrm->u.RecWriteConf.Err.ErrCode != 0)
		{
			qDebug() << ("ErrCode    : 0x%x\n", pCbfPrm->u.RecWriteConf.Err.ErrCode);
			qDebug() << ("ErrDecode  : 0x%x\n", pCbfPrm->u.RecWriteConf.Err.ErrDecode);
			qDebug() << ("ErrCode1   : 0x%x\n", pCbfPrm->u.RecWriteConf.Err.ErrCode1);
			qDebug() << ("ErrCode2   : 0x%x\n", pCbfPrm->u.RecWriteConf.Err.ErrCode2);
			qDebug() << ("AddValue1  : 0x%x\n", pCbfPrm->u.RecWriteConf.Err.AddValue1);
			qDebug() << ("AddValue2  : 0x%x\n", pCbfPrm->u.RecWriteConf.Err.AddValue2);
		}

		if (controller_callback_helper != 0)
			controller_callback_helper->writeReqDone(pCbfPrm->Handle, pCbfPrm->u.RecWriteConf);
	}
}


/*--------------------------------------------------*/
/* useful callbacks                                 */
/*--------------------------------------------------*/


/*-------------------------------------------------------------*/
/* this function will be called from IO-BASE to signal a change*/
/* in the opreation mode                                       */
/*-------------------------------------------------------------*/
void callback_for_mode_change_indication(PNIO_CBE_PRM * pCbfPrm)
{
	/* Check if correct callback type */
	if (pCbfPrm->CbeType == PNIO_CBE_MODE_IND) {
		qDebug() << "PNIO_CBE_MODE_IND for" << pCbfPrm->Handle << ":" << pCbfPrm->u.ModeInd.Mode;
		// forward to helper
		if (controller_callback_helper != 0)
			controller_callback_helper->setMode(pCbfPrm->Handle, pCbfPrm->u.ModeInd.Mode);
	}
	else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_mode_change_indication!";
	}
}

/*-------------------------------------------------------------*/
/* this function will be called from IO-BASE to signal that    */
/* a alarm has been received                                   */
/*-------------------------------------------------------------*/
void callback_for_alarm_indication(PNIO_CBE_PRM * pCbfPrm)
{
	/* Check if correct callback type */
	if (pCbfPrm->CbeType == PNIO_CBE_ALARM_IND) {
		switch (pCbfPrm->u.AlarmInd.pAlarmData->AlarmType) {
		case PNIO_ALARM_DIAGNOSTIC:
			qCritical() << "PNIO_ALARM_DIAGNOSTIC";
			break;

		case PNIO_ALARM_PROCESS:
			qCritical() << "PNIO_ALARM_PROCESS";
			break;

		case PNIO_ALARM_PULL:
			qCritical() << "PNIO_ALARM_PULL";
			break;

		case PNIO_ALARM_PLUG:
			qCritical() << "PNIO_ALARM_PLUG at";
			break;

		case PNIO_ALARM_STATUS:
			qCritical() << "PNIO_ALARM_STATUS";
			break;

		case PNIO_ALARM_UPDATE:
			qCritical() << "PNIO_ALARM_UPDATE";
			break;

		case PNIO_ALARM_REDUNDANCY:
			qCritical() << "PNIO_ALARM_REDUNDACY";
			break;

		case PNIO_ALARM_CONTROLLED_BY_SUPERVISOR:
			qWarning() << "PNIO_ALARM_CONTROLLED_BY_SUPERVISOR";
			break;

		case PNIO_ALARM_RELEASED_BY_SUPERVISOR:
			qDebug() << "PNIO_ALARM_RELEASED_BY_SUPERVISOR";
			break;

		case PNIO_ALARM_PLUG_WRONG:
			qCritical() << "PNIO_ALARM_PLUG_WRONG";
			break;

		case PNIO_ALARM_RETURN_OF_SUBMODULE:
			qDebug() << "PNIO_ALARM_RETURN_OF_SUBMODULE";
			break;

		case PNIO_ALARM_DEV_FAILURE:
			qCritical() << "PNIO_ALARM_DEV_FAILURE";
			break;

		case PNIO_ALARM_DEV_RETURN:
			qDebug() << "PNIO_ALARM_DEV_RETURN";
			break;

		default:
			qCritical() << "callback_for_alarm_indication called with unknown type!";
			break;
		}

		// forward to helper
		if (controller_callback_helper != 0)
			controller_callback_helper->setAlarm(pCbfPrm->Handle, *pCbfPrm->u.AlarmInd.pAlarmData);
	}
	else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_alarm_indication!";
	}
}

/*-------------------------------------------------------------*/
/* this function will be called from IO-BASE to signal that    */
/* a device was activated or deactivated                       */
/*-------------------------------------------------------------*/
void callback_for_device_activation(PNIO_CBE_PRM * pCbfPrm)
{
	if (pCbfPrm->CbeType == PNIO_CBE_DEV_ACT_CONF) {
		switch (pCbfPrm->u.DevActConf.Mode) {
		case PNIO_DA_TRUE:
			qWarning() << "Not Implemented: device activation was send device with Address " << pCbfPrm->u.DevActConf.pAddr->u.Addr << " with result " << pCbfPrm->u.DevActConf.Result;
			break;
		case PNIO_DA_FALSE:
			qWarning() << "Not Implemented: device deactivation was send to device with Address " << pCbfPrm->u.DevActConf.pAddr->u.Addr << " with result " << pCbfPrm->u.DevActConf.Result;
			break;
		}
	}
	else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_device_activation!";
	}
}

void callback_for_stop_request(PNIO_CBE_PRM * pCbfPrm)
{
	if (pCbfPrm->CbeType == PNIO_CBE_CP_STOP_REQ) {
		qWarning() << "Not Implemented: callback_for_stop_request!";
	}
	else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_stop_request!";
	}
}

void callback_for_led_flash(PNIO_CBE_PRM * pCbfPrm)
{
	if (pCbfPrm->CbeType == PNIO_CBE_START_LED_FLASH) {
		qWarning() << "Not Implemented: callback_for_led_flash (PNIO_CBE_START_LED_FLASH)!";
	}
	else if (pCbfPrm->CbeType == PNIO_CBE_STOP_LED_FLASH) {
		qWarning() << "Not Implemented: callback_for_led_flash (PNIO_CBE_STOP_LED_FLASH)!";
	}
	else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_led_flash!";
	}
}

void callback_for_diagnostics(PNIO_CBE_PRM * pCbfPrm)
{
	if (pCbfPrm->CbeType == PNIO_CBE_CTRL_DIAG_CONF) {
		if (pCbfPrm->u.CtrlDiagConf.ErrorCode == PNIO_OK) {
			switch (pCbfPrm->u.CtrlDiagConf.pDiagData->DiagService) {
			case PNIO_CTRL_DIAG_CONFIG_SUBMODULE_LIST:
			{
				// collect config
				std::vector<PNIO_MODULE> config;
				PNIO_CTRL_DIAG_CONFIG_SUBMODULE *modules = (PNIO_CTRL_DIAG_CONFIG_SUBMODULE *)pCbfPrm->u.CtrlDiagConf.pDiagDataBuffer;
				for (PNIO_UINT32 i = 0; i < pCbfPrm->u.CtrlDiagConf.DiagDataBufferLen / sizeof(PNIO_CTRL_DIAG_CONFIG_SUBMODULE); i++) {
					PNIO_CTRL_DIAG_CONFIG_SUBMODULE module = modules[i];
					if (module.DataLength > 0) {
						PNIO_MODULE module_config = {};
						module_config.slot = module.Address.u.Addr;
						module_config.subslot = 0;
                        module_config.station_number = module.StatNo;
                        module_config.hardware_slot = module.Slot;
						module_config.io_type = module.Address.IODataType;
						module_config.data_length = module.DataLength;
						module_config.hw_id = module.HwIdentifier;
						config.push_back(module_config);
					}

                    qDebug() << i
                        << "Station No.:" << module.StatNo
                        << "Slot:" << module.Slot
                        << "SubSlot:" << module.Subslot
                        << "Hardware_ID:" << module.HwIdentifier
                        << "Adress:" << module.Address.u.Addr
                        << "IODatatype:" << (module.Address.IODataType != PNIO_IO_IN ? "OUT" : (modules[i].Address.u.Addr == 65535 ? "DATALESS" : "IN"))
                        << "DataLength:" << module.DataLength;

				}

				//qDebug() << "Received PNIO_CTRL_DIAG_CONFIG_SUBMODULE:" << config;

				// forward to helper
				if (controller_callback_helper != 0)
					controller_callback_helper->setConfig(pCbfPrm->Handle, config);

				break;
			}
			case PNIO_CTRL_DIAG_DEVICE_STATE:
			{
				if (pCbfPrm->u.CtrlDiagConf.DiagDataBufferLen == sizeof(PNIO_CTRL_DIAG_DEVICE)) {
					PNIO_CTRL_DIAG_DEVICE *device = (PNIO_CTRL_DIAG_DEVICE *)pCbfPrm->u.CtrlDiagConf.pDiagDataBuffer;
					qDebug() << "Device:" << device->Mode << device->DiagState << device->Reason;
				}
				else {
					qCritical() << "PNIO_CTRL_DIAG_DEVICE_STATE size should be same:" << pCbfPrm->u.CtrlDiagConf.DiagDataBufferLen << sizeof(PNIO_CTRL_DIAG_DEVICE);
				}
				break;
			}
			case PNIO_CTRL_DIAG_CONFIG_IOROUTER_PRESENT:
			{
				if (pCbfPrm->u.CtrlDiagConf.DiagDataBufferLen == 0) {
					qDebug() << "IO router is not configured, no restrictions for output bits.";
				}
				else {
					qDebug() << "IO router is configured.";
				}
				break;
			}
			case PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE_LIST:
			{
				qDebug() << "PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE_LIST size:" << pCbfPrm->u.CtrlDiagConf.DiagDataBufferLen << sizeof(PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE);
				PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE *slices = (PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE *)pCbfPrm->u.CtrlDiagConf.pDiagDataBuffer;

				for (PNIO_UINT32 i = 0; i < pCbfPrm->u.CtrlDiagConf.DiagDataBufferLen / sizeof(PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE); i++) {
					PNIO_CTRL_DIAG_CONFIG_OUTPUT_SLICE slice = slices[i];
					qDebug() << i << slice.Address.u.Addr << slice.BitOffset << slice.BitLength;
				}
				break;
			}
			/*
			// FW V2.5.2.0 and greater only
		case PNIO_CTRL_DIAG_CONFIG_NAME_ADDR_INFO:
			{
				qDebug() << "PNIO_CTRL_DIAG_CONFIG_NAME_ADDR_INFO size:" << pCbfPrm->CtrlDiagConf.DiagDataBufferLen << sizeof(PNIO_CTRL_DIAG_CONFIG_NAME_ADDR_INFO_DATA);
				break;
			}
			*/
			default:
				qWarning() << "Not Implemented: callback_for_diagnostics for service" << pCbfPrm->u.CtrlDiagConf.pDiagData->DiagService << "!";
			}
		}
		else {
			qCritical() << "Error executing diagnostics service:" << PNIOInterface::errorDescription(pCbfPrm->u.CtrlDiagConf.ErrorCode);
		}
	}
	else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_diagnostics!";
	}
}