#include "pniointerface.h"

static PNIOInterface *pnio_interface = 0;


PNIOInterface::PNIOInterface(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config, const std::vector<std::shared_ptr<PNIORecord>> &record_config) :
    IOInterface()
{
    // register types used in signals and slots
    qRegisterMetaType<PNIO_IOXS>();
	qRegisterMetaType<PNIO_UINT32>();

    // initialize empty device handle
    _handle = 0;
	_ready = false;

	// store config
    updateConfig(in_config, out_config, record_config);

    // invalidate performance timers
    _timer.invalidate();

	if (pnio_interface == 0)
		pnio_interface = this;

    // initialize timer
    _read_timer = new QTimer(this);
    _read_timer->setInterval(1);
    connect(_read_timer, &QTimer::timeout, this, &PNIOInterface::readData);
	_read_timer->start();
}

void PNIOInterface::updateConfig(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config, const std::vector<std::shared_ptr<PNIORecord>> &record_config)
{
    _in_config = in_config;
    _out_config = out_config;

	for (const std::shared_ptr<PNIORecord> &record : record_config) {
		if (std::dynamic_pointer_cast<PNIOWriteRecord>(record)) {
			_write_record_config.push_back(std::dynamic_pointer_cast<PNIOWriteRecord>(record));
		}
		else if (std::dynamic_pointer_cast<PNIOReadRecord>(record)) {
			_read_record_config.push_back(std::dynamic_pointer_cast<PNIOReadRecord>(record));
		}
	}
}

void PNIOInterface::prepareBuffer()
{
    for (const PNIO_MODULE &module : _config) {
        std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = std::make_pair(module.slot, module.subslot);

        // prepare buffer
        if (module.io_type == PNIO_IO_OUT) {
            _out_data[slot_subslot].resize(module.data_length, std::byte('\0'));
        }
    }

    for (const std::shared_ptr<PNIOOutputValue> &out_value : _out_config) {
        int max_byte = out_value->offset() + out_value->size();

		// check if this slot/subslot is included in data
        if (!_out_data.count(out_value->slot_subslot())) {
            qWarning() << "Invalid slot/subslot" << out_value->slot() << out_value->subslot() << "for output" << out_value->name();
        } else if (_out_data[out_value->slot_subslot()].size() <= max_byte) {
            qWarning() << "Invalid size and/or offset for output" << out_value->name();
		}
	}
}

PNIO_UINT32 PNIOInterface::registerIRT()
{
	PNIO_UINT32 errorCode = PNIO_OK;
/*
    // register callback for irt fault
    errorCode = PNIO_CP_register_cbf(_handle, PNIO_CP_CBE_OPFAULT_IND, callback_for_irt);
    if (errorCode != PNIO_OK && errorCode != PNIO_ERR_INVALID_CONFIG) {
        qCritical() << "Error registering PNIO_CP_register_cbf PNIO_CP_OPFAULT_IND!" << errorDescription(errorCode);
        return errorCode;
    }
	
	// register callback for irt start/stop
    errorCode = PNIO_CP_register_cbf(_handle, PNIO_CP_CBE_STARTOP_IND, callback_for_irt);
    if (errorCode != PNIO_OK && errorCode != PNIO_ERR_INVALID_CONFIG) {
        qCritical() << "Error registering PNIO_CP_register_cbf PNIO_CP_CBE_STARTOP_IND!" << errorDescription(errorCode);
        return errorCode;
    }
*/
	return errorCode;
}

void PNIOInterface::startTimer()
{
	// check if timer already started
	if (!_timerMutex.tryLock()) {
		return;
	}

	// this is very time consuming. reserve one thread from global pool.
	QThreadPool::globalInstance()->reserveThread();

}

void PNIOInterface::stopTimer()
{
    if (!_timerMutex.tryLock()) {   // make sure timer was already locked
        // release one thread
        QThreadPool::globalInstance()->releaseThread();
    }
    
//#if defined(Q_OS_MACX)
    if (_read_timer->isActive()) {
        _read_timer->stop();
    }
//#endif

    // unlock timer mutex
    _timerMutex.unlock();
}

void PNIOInterface::irtCycle(PNIO_UINT32 handle)
{
    if (handle == _handle)
        readData();
}

void PNIOInterface::readData()
{
#ifdef DEBUG
#if defined(Q_OS_WIN)
	DWORD prio = GetThreadPriority(GetCurrentThread());
	if (prio != 15)
		qDebug() << "Wrong Thread Priority!" << prio;
#endif
#endif

    int64_t read_delay = _timer.isValid() ? _timer.nsecsElapsed() : 0;
    _timer.restart();

    readDataInternal();

    int64_t read_duration = _timer.nsecsElapsed();

    _read_performance(read_duration);
    if (read_delay > 0) {
        _read_jitter(read_delay - (1 * 1000000));

		_min_jitter = extract_result< tag::min >(_read_jitter);
		_mean_jitter = mean(_read_jitter);
		_max_jitter = extract_result< tag::max >(_read_jitter);
        _variance_jitter = extract_result< tag::variance >(_read_jitter);

		_min_performance = extract_result< tag::min >(_read_performance);
		_mean_performance = mean(_read_performance);
		_max_performance = extract_result< tag::max >(_read_performance);
        _variance_performance = extract_result< tag::variance >(_read_performance);
    }
}

void PNIOInterface::printStatistics()
{
    if (_max_jitter != 0) {
        qDebug() << "PNIO jitter min:" << _min_jitter / 1000000.0 << "avg:" << _mean_jitter / 1000000.0 << "+-" << std::sqrt(_variance_jitter) / 1000000.0 << "max:" << _max_jitter / 1000000.0;
        qDebug() << "PNIO performance min:" << _min_performance / 1000000.0 << "avg:" << _mean_performance / 1000000.0 << "+-" << std::sqrt(_variance_performance) / 1000000.0 << "max:" << _max_performance / 1000000.0;
    }
}

void PNIOInterface::processData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data, const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> &state)
{
    // parse buffer according to config
    for (const std::shared_ptr<PNIOInputValue> &config : _in_config) {
        std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = config->slot_subslot();
        if (data.count(slot_subslot) && data.at(slot_subslot).size() >= config->offset() + config->size()) {
            config->update(data.at(slot_subslot), state.at(slot_subslot) == PNIO_S_GOOD);
        } else if (state.count(slot_subslot)) {
            config->set_valid(state.at(slot_subslot) == PNIO_S_GOOD);
        } else {
            //qDebug() << "Ignore" << config->slot() << config->subslot() << "offset:" << config->offset() << config->size() << "data size:" << (data.count(slot_subslot) ? data.at(slot_subslot).size() : 0);
        }
    }
    
    // run main logic loop
    if (auto logic = _logic.lock()) {
        logic->doWorkInternal();
    } else {
        writeOutputs();
    }
}

void PNIOInterface::writeOutputs()
{
    // prepare buffer here and then copy to device
    std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> data = _out_data;
    for (const std::shared_ptr<PNIOOutputValue> &config : _out_config) {
        std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = config->slot_subslot();
        config->update(data[slot_subslot]);
    }
    
    // write data
    auto states = writeData(data);
    
    // update out states
    for (const std::shared_ptr<PNIOOutputValue> &config : _out_config) {
        std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = config->slot_subslot();
        config->set_valid(states[slot_subslot] == PNIO_S_GOOD);
    }
}

QList<PluginInterface::Message> PNIOInterface::alarmMessages() const
{
    QList<PluginInterface::Message> messages;

    if (!isInitialized()) {
        messages << PluginInterface::Message(60000, tr("ProfiNet not initialized"), QtWarningMsg);
    }

    if (_current_state != PluginInterface::MachineState::Off) {
        for (auto &module_error : _module_errors) {

            auto statNo = module_error.first.first;
            auto slot = module_error.first.second;
            auto error_mask = module_error.second;

            if (error_mask & connectionLost) {
                messages << PluginInterface::Message(60100 + (statNo * 10) + slot, tr("Connection to Station %1 lost").arg(statNo), QtFatalMsg);
            }
            if (error_mask & moduleUnplugged) {
                messages << PluginInterface::Message(60200 + (statNo * 10) + slot, tr("Station %1 Module %2 unplugged").arg(statNo).arg(slot), QtFatalMsg);
            }
            if (error_mask & moduleReadErr) {
                messages << PluginInterface::Message(60300 + (statNo * 10) + slot, tr("Station %1 Module %2 read error").arg(statNo).arg(slot), QtFatalMsg);
            }
            if (error_mask & moduleWriteErr) {
                messages << PluginInterface::Message(60400 + (statNo * 10) + slot, tr("Station %1 Module %2 write error").arg(statNo).arg(slot), QtFatalMsg);
            }
        }
    }

    return messages;
}

bool PNIOInterface::hasError() const
{
    bool hasError = false;
    for (auto &module_error : _module_errors) {
        hasError = hasError || module_error.second != 0;
    }
    return hasError;
}

void PNIOInterface::setLogic(std::shared_ptr<Logic> logic)
{
    _logic = logic;
    moveToThread(logic->thread());
}

const QString PNIOInterface::errorDescription(const PNIO_UINT32 errorCode)
{
    QString strError;
    switch(errorCode)
    {
    case PNIO_OK:								strError = tr("success");																													break;

    // warnings
    case PNIO_WARN_IRT_INCONSISTENT:			strError = tr("IRT Data may be inconsistent");																								break;
    case PNIO_WARN_NO_SUBMODULES:				strError = tr("no submodules to be updated");																								break;
    case PNIO_WARN_LOCAL_STATE_BAD:				strError = tr("data was written with local state PNIO_S_BAD, because not all components of splitted module have local state PNIO_S_GOOD");	break;

    // wrong functions calls, parameter errors
    case PNIO_ERR_PRM_HND:						strError = tr("parameter Handle is illegal");                                                                                               break;
    case PNIO_ERR_PRM_BUF:						strError = tr("parameter buffer is NULL-Ptr");                                                                                              break;
    case PNIO_ERR_PRM_LEN:						strError = tr("parameter length is wrong");                                                                                                 break;
    case PNIO_ERR_PRM_ADD:						strError = tr("parameter address is wrong");                                                                                                break;
    case PNIO_ERR_PRM_RSTATE:					strError = tr("parameter remote state is NULL-Ptr");                                                                                        break;
    case PNIO_ERR_PRM_CALLBACK:					strError = tr("parameter cbf is illegal");                                                                                                  break;
    case PNIO_ERR_PRM_TYPE:						strError = tr("parameter type has no valid value");                                                                                         break;
    case PNIO_ERR_PRM_EXT_PAR:					strError = tr("parameter ExtPar has no valid value");                                                                                       break;
    case PNIO_ERR_PRM_IO_TYPE:					strError = tr("parameter PNIO_ADDR::IODataType is wrong");                                                                                  break;
    case PNIO_ERR_PRM_CP_ID:					strError = tr("parameter CpIndex is wrong, probably driver is not loaded");                                                                 break;
    case PNIO_ERR_PRM_LOC_STATE:				strError = tr("parameter IOlocState has no valid value");                                                                                   break;
    case PNIO_ERR_PRM_REC_INDEX:				strError = tr("parameter RecordIndex has no valid value");                                                                                  break;
    case PNIO_ERR_PRM_TIMEOUT:					strError = tr("parameter timeout has no valid value");                                                                                      break;
    case PNIO_ERR_PRM_DEV_ANNOTATION:			strError = tr("parameter annotation has no valid value");                                                                                   break;
    case PNIO_ERR_PRM_DEV_STATE:				strError = tr("parameter state has no valid value");                                                                                        break;
    case PNIO_ERR_PRM_PCBF:						strError = tr("parameter pCbf has no valid value");                                                                                         break;
    case PNIO_ERR_PRM_MAX_AR_VALUE:				strError = tr("parameter MaxAR has no valid value");                                                                                        break;
    case PNIO_ERR_PRM_ACCESS_TYPE:				strError = tr("parameter AccessType has no valid value");                                                                                   break;
    case PNIO_ERR_PRM_POINTER:					strError = tr("an invalid pointer was passed");                                                                                             break;
    case PNIO_ERR_PRM_INVALIDARG:				strError = tr("an invalid argument was passed");                                                                                            break;
    case PNIO_ERR_PRM_MEASURE_NUMBER:			strError = tr("wrong Measure No in cycle statistics, must be -1 (actual measure) up to 49");                                                break;
    case PNIO_ERR_PRM_CYCLE_OFFSET:				strError = tr("wrong Offset for cycle info buffer (must be 0 to 19)");                                                                      break;
    case PNIO_ERR_PRM_ROUTER_ADD:				strError = tr("address used by io router");                                                                                                 break;

    // instance errors
    case PNIO_ERR_WRONG_HND:					strError = tr("unknown handle");                                                                                                            break;
    case PNIO_ERR_MAX_REACHED:					strError = tr("maximal number of opens reached; close unused applications");                                                                break;
    case PNIO_ERR_CREATE_INSTANCE:				strError = tr("fatal error, reboot your system! (PNIO not available or PNIO Driver Outdated)");                                             break;
    case PNIO_ERR_MODE_VALUE:					strError = tr("parameter mode has no valid value");                                                                                         break;
    case PNIO_ERR_OPFAULT_NOT_REG:				strError = tr("register OPFAULT callback before register STARTOP callback");                                                                break;
    case PNIO_ERR_NEWCYCLE_SEQUENCE_REG:		strError = tr("register NEWCYCLE callback before register STARTOP callback");                                                               break;
    case PNIO_ERR_NETWORK_PROT_NOT_AVAILABLE:	strError = tr("network protocol not available, check card configuration");                                                                  break;

    // other errors
    case PNIO_ERR_NO_CONNECTION:				strError = tr("device data not available, because device is not connected to controller");                                                  break;
    case PNIO_ERR_OS_RES:						strError = tr("fatal error, no more operation system resources available");                                                                 break;
    case PNIO_ERR_ALREADY_DONE:					strError = tr("action was already performed");                                                                                              break;
    case PNIO_ERR_NO_CONFIG:					strError = tr("no configuration for this index available");                                                                                 break;
    case PNIO_ERR_SET_MODE_NOT_ALLOWED:			strError = tr("PNIO_set_mode not allowed, use PNIO_CEP_MODE_CTRL by PNIO_controller_open");                                                 break;
    case PNIO_ERR_DEV_ACT_NOT_ALLOWED:			strError = tr("PNIO_device_activate not allowed, use PNIO_CEP_MODE_CTRL by PNIO_controller_open");                                          break;
    case PNIO_ERR_NO_LIC_SERVER:				strError = tr("licence server not running, check your installation");                                                                       break;
    case PNIO_ERR_VALUE_LEN:					strError = tr("wrong length value");                                                                                                        break;
    case PNIO_ERR_SEQUENCE:						strError = tr("wrong calling sequence");                                                                                                    break;
    case PNIO_ERR_INVALID_CONFIG:				strError = tr("invalid configuration, check your configuration");                                                                           break;
    case PNIO_ERR_UNKNOWN_ADDR:					strError = tr("address unknown in configuration, check your configuration");                                                                break;
    case PNIO_ERR_NO_RESOURCE:					strError = tr("no resouce too many requests been processed");                                                                               break;
    case PNIO_ERR_CONFIG_IN_UPDATE:				strError = tr("configuration update is in progress or CP is in STOP state, try again later");                                               break;
    case PNIO_ERR_NO_FW_COMMUNICATION:			strError = tr("no communication with firmware, reset cp or try again later");                                                               break;
    case PNIO_ERR_STARTOP_NOT_REGISTERED:		strError = tr("no synchronous function allowed, use PNIO_CEP_SYNC_MODE by PNIO_controller_open or PNIO_device_open");                       break;
    case PNIO_ERR_OWNED:						strError = tr("interface-submodule cannot be removed because it is owned by an AR");                                                        break;
    case PNIO_ERR_START_THREAD_FAILED:			strError = tr("failed to start thread, probably by lack of pthread resources");                                                             break;
    case PNIO_ERR_START_RT_THREAD_FAILED:		strError = tr("failed to start realtime thread, probably you need root capability to do it");                                               break;
    case PNIO_ERR_DRIVER_IOCTL_FAILED:			strError = tr("failed to ioctl driver, probably API version mismatch");                                                                     break;
    case PNIO_ERR_AFTER_EXCEPTION:				strError = tr("exception ocurred, save exception info (see manual) and reset cp");                                                          break;
    case PNIO_ERR_NO_CYCLE_INFO_DATA:			strError = tr("no cycle data available");                                                                                                   break;
    case PNIO_ERR_SESSION:						strError = tr("request belongs to an old session");                                                                                         break;
    case PNIO_ERR_ALARM_DATA_FORMAT:			strError = tr("wrong format of alarm data");                                                                                                break;
    case PNIO_ERR_ABORT:						strError = tr("operation was aborted");                                                                                                     break;
    case PNIO_ERR_CORRUPTED_DATA:				strError = tr("datas are corrupter or have wrong format");                                                                                  break;
    case PNIO_ERR_FLASH_ACCESS:					strError = tr("error by flash operations");                                                                                                 break;
    case PNIO_ERR_WRONG_RQB_LEN:				strError = tr("wrong length of request block at firmware interface, firmware not compatible to host sw");                                   break;
    case PNIO_ERR_NO_RESET_VERIFICATION:		strError = tr("reset request was sendet to firmware, but firmware rut up can't be verified");                                               break;

    // internal error
    case PNIO_ERR_INTERNAL:                     strError = tr("fatal error, contact SIEMENS hotline");                                                                                      break;

    default:									strError = tr("enknown error");                                                                                                             break;
    }

    return QString("Error #0x%1: %2").arg(errorCode, 8, 16, QChar('0')).arg(strError);
}

/*----------------------------------------------------------------------------------------------------*/
/*    CALLBACKS                                                                                       */
/*----------------------------------------------------------------------------------------------------*/

void callback_for_irt(PNIO_CP_CBE_PRM * pCbfPrm)
{
	if (pCbfPrm->CbeType == PNIO_CP_CBE_STARTOP_IND) {
		qWarning() << "Not Implemented: irt_callback (PNIO_CP_CBE_STARTOP_IND)!";
	} else if (pCbfPrm->CbeType == PNIO_CP_CBE_OPFAULT_IND) {
		qWarning() << "Not Implemented: irt_callback (PNIO_CP_CBE_OPFAULT_IND)!";
	} else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_irt!";
	}
}

void callback_for_irt_cycle(PNIO_CP_CBE_PRM * pCbfPrm)
{
	if (pCbfPrm->CbeType == PNIO_CP_CBE_NEWCYCLE_IND) {
#ifdef Q_OS_WIN
       SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

		//qWarning() << "Cycle:" << pCbfPrm->u.NewCycle.CycleInfo.ClockCount << pCbfPrm->u.NewCycle.CycleInfo.CycleCount << pCbfPrm->u.NewCycle.CycleInfo.CountSinceCycleStart;
		if (pnio_interface)
			pnio_interface->readData();
	} else {
		qCritical() << "Wrong callback type" << pCbfPrm->CbeType << "for callback_for_irt_cycle!";
	}
}
