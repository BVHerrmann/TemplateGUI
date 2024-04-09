#include "pniodevice.h"

#if 0
// define callbacks
PNIO_IOXS PNIOD_cbf_data_write(PNIO_UINT32 DevHndl, PNIO_DEV_ADDR *pAddr, PNIO_UINT32 BufLen, PNIO_UINT8 *pBuffer, PNIO_IOXS Iocs);
PNIO_IOXS PNIOD_cbf_data_read(PNIO_UINT32 DevHndl, PNIO_DEV_ADDR *pAddr, PNIO_UINT32 BufLen, PNIO_UINT8 *pBuffer, PNIO_IOXS Iops);
void PNIOD_cbf_async_rec_read(PNIOD_CBF_ASYNC_REC_READ_PARAMS_TYPE *pParams);
void PNIOD_cbf_async_rec_write(PNIOD_CBF_ASYNC_REC_WRITE_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_alarm_done(PNIOD_CBF_SYNC_ALARM_DONE_PARAMS_TYPE *pParams);
void PNIOD_cbf_async_connect_ind(PNIOD_CBF_ASYNC_CONNECT_IND_PARAMS_TYPE *pParams);
void PNIOD_cbf_async_ownership_ind(PNIOD_CBF_ASYNC_OWNERSHIP_IND_PARAMS_TYPE *pParams);
void PNIOD_cbf_async_indata_ind(PNIOD_CBF_ASYNC_INDATA_IND_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_disconnect_ind(PNIOD_CBF_SYNC_DISCONNECT_IND_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_data_status_ind(PNIOD_CBF_SYNC_DATA_STATUS_IND_PARAMS_TYPE *pParams);
void PNIOD_cbf_async_prm_end_ind(PNIOD_CBF_ASYNC_PRM_END_IND_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_device_stopped(PNIOD_CBF_SYNC_STOPPED_PARAMS_TYPE *pParams);
void PNIOD_cbf_async_irt_init_inputs(PNIOD_CBF_ASYNC_IRT_INIT_INPUTS_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_cp_stop_req(PNIOD_CBF_SYNC_CP_STOP_REQ_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_start_led_flash(PNIOD_CBF_SYNC_START_LED_FLASH_PARAMS_TYPE *pParams);
void PNIOD_cbf_sync_stop_led_flash(PNIOD_CBF_SYNC_STOP_LED_FLASH_PARAMS_TYPE *pParams);

// static device pointer for callbacks
static PNIODevice *device = 0;


PNIODevice::PNIODevice(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config) :
    PNIOInterface(in_config, out_config)
{
    // store device reference
    device = this;

	// initialize buffer
	prepareBuffer();
}

PNIODevice::~PNIODevice()
{
    device = 0;
}

void PNIODevice::connectPNIO()
{
    PNIO_UINT32 errorCode = PNIO_OK;

    errorCode = initialize();
    if (errorCode != PNIO_OK)
        return;

    // read config from device
    readConfig();

    errorCode = startDevice();
    if (errorCode != PNIO_OK)
        return;

    startTimer();
}

void PNIODevice::disconnectPNIO()
{
    stopTimer();

    stopDevice();

    uninitialize();
}

void PNIODevice::readConfig()
{
    PNIOD_GET_CONFIG_SYNC_PARAMS_TYPE *config = nullptr;
    
    config = PNIOD_get_config_sync(_handle);
    if (config != nullptr) {
        qDebug() << QString("Read configuration: VendorId = 0x%1, DeviceId = 0x%2.").arg(config->VendorId, 4, 16, QLatin1Char('0')).arg(config->DeviceId, 4, 16, QLatin1Char('0'));

        qDebug() << QString("     %1 |  %2 |  %3 |  %4 |  %5 |  %6").arg("Slot", 10).arg("Subslot", 10).arg("ModIdent", 10).arg("SubIdent", 10).arg("InLen", 10).arg("OutLen", 10);
        for (int i = 0; i < config->NrOfSubmods; i++) {
            qDebug() << QString("     %1 |  %2 |  0x%3 |  0x%4 |  %5 |  %6").arg(config->pSubmodList[i].Addr.u.Geo.Slot, 10).arg(config->pSubmodList[i].Addr.u.Geo.Subslot, 10).arg(config->pSubmodList[i].ModIdent, 8, 16, QLatin1Char('0')).arg(config->pSubmodList[i].SubIdent, 8, 16, QLatin1Char('0')).arg(config->pSubmodList[i].InDatLength, 10).arg(config->pSubmodList[i].OutDatLength, 10);
        }


        // collect config
        std::vector<PNIO_MODULE> pnio_config;
        for (int i = 0; i < config->NrOfSubmods; i++) {
            PNIOD_GET_CONFIG_SYNC_ELEMENT_TYPE module = config->pSubmodList[i];

            if (module.InDatLength > 0 || module.OutDatLength > 0) {
                PNIO_MODULE module_config = {};
                module_config.slot = module.Addr.u.Geo.Slot;
                module_config.subslot = module.Addr.u.Geo.Subslot;
                module_config.io_type = module.Addr.IODataType;
                module_config.data_length = module.InDatLength > module.OutDatLength  ? module.InDatLength : module.OutDatLength;

                module_config.mod_id = module.ModIdent;
                module_config.subslot_id = module.SubIdent;
                module_config.api = module.Api;

                if (module.InDatLength > 0 && module.OutDatLength > 0) {
                    module_config.io_type = PNIO_IO_IN_OUT;
                } else if (module.InDatLength > 0) {
                    module_config.io_type = PNIO_IO_IN;
                } else if (module.OutDatLength > 0) {
                    module_config.io_type = PNIO_IO_OUT;
                } else {
                    module_config.io_type = PNIO_IO_UNKNOWN;
                }

                pnio_config.push_back(module_config);
            }

            // set new config
            _config = pnio_config;

            // initialize buffers
            prepareBuffer();
        }
    } else {
        qWarning() << "Failed to read config!";
    }
}

void PNIODevice::readDataInternal()
{
    if (!_ready)
        return;

    PNIO_UINT32 errorCode = PNIOD_trigger_data_read_sync(_handle, nullptr, PNIO_ACCESS_RT_WITH_LOCK);
    if(errorCode != PNIO_OK)
        qWarning() << tr("Error in PNIO_initiate_data_read.") << errorDescription(errorCode);

    // at this point all callback from reading have finished and new output data is available
    // initiate writing data
    errorCode = PNIOD_trigger_data_write_sync(_handle, nullptr, PNIO_ACCESS_RT_WITH_LOCK);
    if (errorCode != PNIO_OK)
        qWarning() << tr("Error in PNIO_initiate_data_write.") << errorDescription(errorCode);
}

std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> PNIODevice::writeData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data)
{
    //!: readDataInternal actually performs the writing as this is calles as part of the read data callback and writing here would cause a deadlock!
    assert(data.size() == _out_data.size());

    // store new data
    _out_data = data;
    
    return _out_state;
}

PNIO_IOXS PNIODevice::cbf_data_write(
        const PNIO_DEV_ADDR* pAddr,              /* [in] geographical address */
        const PNIO_UINT32    BufLen,             /* [in] length of the submodule input data */
        PNIO_UINT8*          pBuffer,            /* [out] Ptr to data buffer to write to */
        const PNIO_IOXS      Iocs)               /* [in] remote (io controller) consumer status */
{
    PNIO_IOXS state = PNIO_S_GOOD;
    std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = std::make_pair(pAddr->u.Geo.Slot, pAddr->u.Geo.Subslot);

    _out_state[slot_subslot] = Iocs; // consumer status (of remote IO controller)
    
    if (BufLen == 0) {
        state = PNIO_S_GOOD;
    } else if (BufLen <= _out_data[slot_subslot].size()) {
        // copy the application data to the stack
        memcpy(pBuffer, _out_data[slot_subslot].data(), BufLen);
        state = PNIO_S_GOOD;    // assume everything is ok

#ifdef DEBUG
        QString bufferContent;
        for (PNIO_UINT32 i = 0; i < BufLen; i++)
            bufferContent.append(QString("%1").arg(pBuffer[i], 2, 16, QChar('0')));
        qDebug() << "Write IO Data buffer:" << pAddr->u.Geo.Slot << pAddr->u.Geo.Subslot << bufferContent;
#endif
    } else {
        qWarning() << "Configuration error! BufLen:" << BufLen << "to big for slot:" << pAddr->u.Geo.Slot << "subslot:" << pAddr->u.Geo.Subslot;

        state = PNIO_S_BAD; // set local status to bad
    }

    return state; // return local provider status
}

PNIO_IOXS PNIODevice::cbf_data_read(
        const PNIO_DEV_ADDR* pAddr,              /* [in] geographical address */
        const PNIO_UINT32    BufLen,             /* [in] length of the submodule input data */
        const PNIO_UINT8*    pBuffer,            /* [in] Ptr to data buffer to read from */
        const PNIO_IOXS      Iops)
{
    PNIO_IOXS local_state = PNIO_S_BAD;
    
    if (BufLen > 0) {
        std::pair<PNIO_UINT32, PNIO_UINT32> slot_subslot = std::make_pair(pAddr->u.Geo.Slot, pAddr->u.Geo.Subslot);
        std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> data { {slot_subslot, std::vector<std::byte>(BufLen, std::byte('\0'))} };
        std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> state { {slot_subslot, Iops} };
        
        // copy the data from the stack to the application buffer
        memcpy(data[slot_subslot].data(), pBuffer, BufLen);
        local_state = PNIO_S_GOOD; // assume everything is ok
        
        // process data
        processData(data, state);
    }

    // consumer state (of local IO device)
    return local_state;
}

void PNIODevice::cbf_async_indata_ind(PNIOD_CBF_ASYNC_INDATA_IND_PARAMS_TYPE *pParams)
{
    _indata_ind_semaphore.release();
    _ready = true;
}

void PNIODevice::cbf_async_prm_end_ind(PNIOD_CBF_ASYNC_PRM_END_IND_PARAMS_TYPE *pParams)
{
    /* now write to DI modules (input from the controller point of view, output from our point of view) */
    qDebug() << "Writing to all input modules.";
    /* write to all submodules */
    PNIO_UINT32 result = PNIOD_trigger_data_write_sync(_handle, nullptr, PNIO_ACCESS_RT_WITH_LOCK);
    if (result != PNIO_OK) {
        qDebug() << QString("Error while writing data: 0x%1.").arg(result, 8, 16);
    }
    
    /* now read from DO modules (output from the controller point of view, input from our point of view) */
    qDebug() << "Reading from all output modules.";
    /* read from all submodules */
    result = PNIOD_trigger_data_read_sync(_handle, nullptr, PNIO_ACCESS_RT_WITH_LOCK);
    if (result != PNIO_OK) {
        qDebug() << QString("Error while reading data: 0x%1.").arg(result, 8, 16);
    }
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Initialize()                                                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*      This function does the initialization of the PNIO            */
/*      device. Registration of callbacks is part of initialization  */
/*-------------------------------------------------------------------*/
PNIO_UINT32 PNIODevice::initialize()
{
    PNIOD_CBF_FUNCTIONS  structCBFunctions;
    PNIOD_OPEN_SYNC_PARAMS_TYPE *openSyncParams = 0;
    PNIO_UINT32         handle = 0;             /*0 is invalid handle */
    PNIO_UINT32         errorCode = PNIO_ERR_INTERNAL;

    /* initialize the parameter for 'open' */
    if ((errorCode = PNIOD_init_open_sync(&openSyncParams, 1, &handle)) != PNIO_OK)
    {
        qWarning() << "Error occured during intializing open:" << errorCode;
        return errorCode;
    }
    if (!openSyncParams)
    {
        qWarning() << "Error because of PNIO_init_open_sync did not fill up openSyncParams (NULL)!";
        return PNIO_ERR_INTERNAL;
    }

    /* assign callback functions */
    openSyncParams->pCbf = &structCBFunctions;
    memset(&structCBFunctions, 0, sizeof(PNIOD_CBF_FUNCTIONS));
    structCBFunctions.size                          = sizeof(PNIOD_CBF_FUNCTIONS);
    structCBFunctions.cbf_data_write                = PNIOD_cbf_data_write;
    structCBFunctions.cbf_data_read                 = PNIOD_cbf_data_read;
    structCBFunctions.cbf_async_rec_read            = PNIOD_cbf_async_rec_read;
    structCBFunctions.cbf_async_rec_write           = PNIOD_cbf_async_rec_write;
    structCBFunctions.cbf_sync_alarm_done           = PNIOD_cbf_sync_alarm_done;
    structCBFunctions.cbf_async_connect_ind         = PNIOD_cbf_async_connect_ind;
    structCBFunctions.cbf_async_ownership_ind       = PNIOD_cbf_async_ownership_ind;
    structCBFunctions.cbf_async_indata_ind          = PNIOD_cbf_async_indata_ind;
    structCBFunctions.cbf_sync_disconnect_ind       = PNIOD_cbf_sync_disconnect_ind;
    structCBFunctions.cbf_sync_data_status_ind      = PNIOD_cbf_sync_data_status_ind;
    structCBFunctions.cbf_async_prm_end_ind         = PNIOD_cbf_async_prm_end_ind;
    structCBFunctions.cbf_sync_device_stopped       = PNIOD_cbf_sync_device_stopped;
    structCBFunctions.cbf_async_irt_init_inputs     = PNIOD_cbf_async_irt_init_inputs;
    structCBFunctions.cbf_sync_cp_stop_req          = PNIOD_cbf_sync_cp_stop_req;
    structCBFunctions.cbf_sync_start_led_flash      = PNIOD_cbf_sync_start_led_flash;
    structCBFunctions.cbf_sync_stop_led_flash       = PNIOD_cbf_sync_stop_led_flash;

    /* fill device annotation info */
    memset(&openSyncParams->DevAnnotation.DeviceType, ' ', sizeof(openSyncParams->DevAnnotation.DeviceType));
    memcpy(&openSyncParams->DevAnnotation.DeviceType, &ANNOT_NAME, sizeof(ANNOT_NAME) - 1);
    memset(&openSyncParams->DevAnnotation.OrderId, ' ', sizeof(openSyncParams->DevAnnotation.OrderId));
    memcpy(&openSyncParams->DevAnnotation.OrderId, &ANNOT_ORDERID, sizeof(ANNOT_ORDERID) - 1);
    openSyncParams->DevAnnotation.HwRevision = ANNOT_HW_REV;
    openSyncParams->DevAnnotation.SwRevisionPrefix = ANNOT_SW_PREFIX;
    openSyncParams->DevAnnotation.SwRevision1 = ANNOT_SW_REV_1;
    openSyncParams->DevAnnotation.SwRevision2 = ANNOT_SW_REV_2;
    openSyncParams->DevAnnotation.SwRevision3 = ANNOT_SW_REV_3;

    /* open device */
    qDebug() << tr("Initializing PNIO Device");
    errorCode = PNIOD_open_sync(&openSyncParams);
    if (errorCode != PNIO_OK)
    {
        qWarning() << tr("Error initializing the PNIO Device.") << errorDescription(errorCode);
        return errorCode;
    }

    _handle = handle;
    
	// register IRT callbacks
	errorCode = registerIRT();
	
	return  errorCode;
}

/*------------------------------------------------------------------*/
/*                                                                  */
/*  Uninitialize()                                                  */
/*                                                                  */
/*------------------------------------------------------------------*/
/*      This function does the de-initialization of the PNIO device */
/*------------------------------------------------------------------*/
PNIO_UINT32 PNIODevice::uninitialize()
{
    PNIO_UINT32 errorCode = PNIOD_close_sync(_handle);

    if (errorCode != PNIO_OK)
        qWarning() << tr("Error closing the device.") << errorDescription(errorCode);

    return errorCode;
}

PNIO_UINT32 PNIODevice::startDevice()
{
    PNIO_UINT32 errorCode = PNIOD_start_sync(_handle);
    
    if(errorCode != PNIO_OK) {
        PNIOD_stop_async(_handle);
        qWarning() << tr("Error in PNIO_device_start.") << errorDescription(errorCode);
        return errorCode;
    }

    // wait for start of data exchange
    qDebug() << "Waiting for InData callback.";
    _indata_ind_semaphore.acquire();
    qDebug() << "InData callback received.";

    return PNIO_OK;
}

PNIO_UINT32 PNIODevice::stopDevice()
{
    _ready = false;

    PNIO_UINT32 errorCode = PNIOD_stop_async(_handle);

    if(errorCode != PNIO_OK) {
        qWarning() << tr("Error in PNIO_device_stop.") << errorDescription(errorCode);
        return errorCode;
    }

    // wait until device is actually stopped
    qDebug() << "Waiting for stop of Device.";
	_device_stopped_semaphore.acquire();
    qDebug() << "Device stop callback received.";

    return  errorCode;
}

void PNIODevice::cbf_sync_device_stopped(PNIOD_CBF_SYNC_STOPPED_PARAMS_TYPE *pParams)
{
	_device_stopped_semaphore.release();
}

/*----------------------------------------------------------------------------------------------------*/
/*    CALLBACKS                                                                                       */
/*----------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
/* Relevant callback functions for Initialize and UnInitialize of a PNIO device */
/* are defined here.                                                            */
/*------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------*/
/*                                                                               */
/*  PNIO_cbf_data_write (...)                                                    */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
/*                                                                               */
/*  Passes the input data from the application to the stack.                     */
/*  The application reads the data from the specified input module               */
/*  and handles it to the stack.                                                 */
/*  The function UsrReadInputData() is called by the PNIO stack.                 */
/*                                                                               */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
PNIO_IOXS PNIOD_cbf_data_write(
        PNIO_UINT32    DevHndl,            /* [in] Handle for device */
        PNIO_DEV_ADDR* pAddr,              /* [in] geographical address */
        PNIO_UINT32    BufLen,             /* [in] length of the submodule input data */
        PNIO_UINT8*    pBuffer,            /* [out] Ptr to data buffer to write to */
        PNIO_IOXS      Iocs)               /* [in] remote (io controller) consumer status */
{
    if (Iocs != PNIO_S_GOOD)
        qWarning() << QString("Iocs NOT GOOD: PNIOD_cbf_data_write(..., len=%1, Iocs=%2) for devHandle 0x%3, slot %4, subslot %5").arg(BufLen).arg(Iocs).arg(DevHndl).arg(pAddr->u.Geo.Slot).arg(pAddr->u.Geo.Subslot);

    return device->cbf_data_write(pAddr, BufLen, pBuffer, Iocs);
}

/*-------------------------------------------------------------------------------*/
/*                                                                               */
/*  PNIO_cbf_data_read (...)                                                     */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
/*                                                                               */
/*  Passes the output data from the stack to the application.                    */
/*  The application takes the data and writes it to the specified                */
/*  output module.                                                               */
/*  function UsrWriteOutputData() is called by the PNIO stack.                   */
/*                                                                               */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
PNIO_IOXS PNIOD_cbf_data_read(
        PNIO_UINT32    DevHndl,            /* [in] Handle for Multidevice */
        PNIO_DEV_ADDR* pAddr,              /* [in] geographical address */
        PNIO_UINT32    BufLen,             /* [in] length of the submodule input data */
        PNIO_UINT8*    pBuffer,            /* [in] Ptr to data buffer to read from */
        PNIO_IOXS      Iops)               /* [in] (io controller) provider status */
{
    if (Iops != PNIO_S_GOOD)
        qWarning() << QString("Iops NOT GOOD: PNIOD_cbf_data_read(..., len=%1, Iops=%2) for devHandle 0x%3, slot %4, subslot %5").arg(BufLen).arg(Iops).arg(DevHndl).arg(pAddr->u.Geo.Slot).arg(pAddr->u.Geo.Subslot);
    
    return device->cbf_data_read(pAddr, BufLen, pBuffer, Iops);
}

void PNIOD_cbf_async_rec_read(PNIOD_CBF_ASYNC_REC_READ_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;

    auto future = std::async(std::launch::async, [=] {
        PNIOD_rec_read_async_rsp(pParams);
    });
}

void PNIOD_cbf_async_rec_write(PNIOD_CBF_ASYNC_REC_WRITE_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;

    auto future = std::async(std::launch::async, [=] {
        PNIOD_rec_write_async_rsp(pParams);
    });
}

void PNIOD_cbf_sync_alarm_done(PNIOD_CBF_SYNC_ALARM_DONE_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;
}

void PNIOD_cbf_async_connect_ind(PNIOD_CBF_ASYNC_CONNECT_IND_PARAMS_TYPE *pParams)
{
    long ip = qFromBigEndian(pParams->HostIp);
    qDebug() << QString("> Session %1: Connection request received from %2.%3.%4.%5.").arg(pParams->SessionKey).arg((ip >> 24) & 0xff).arg((ip >> 16) & 0xff).arg((ip >> 8) & 0xff).arg(ip & 0xff);
    
    auto future = std::async(std::launch::async, [=] {
        PNIOD_connect_async_rsp(pParams);
    });
}

void PNIOD_cbf_async_ownership_ind(PNIOD_CBF_ASYNC_OWNERSHIP_IND_PARAMS_TYPE *pParams)
{
    qDebug() << QString("> Session %1: Submodule ownership has changed: %2.").arg(pParams->SessionKey).arg(pParams->ArContext);

    auto future = std::async(std::launch::async, [=] {
        PNIOD_ownership_async_rsp(pParams);
    });
}

void PNIOD_cbf_async_indata_ind(PNIOD_CBF_ASYNC_INDATA_IND_PARAMS_TYPE *pParams)
{
    qDebug() << QString("> Session %1: Data exchange started.").arg(pParams->SessionKey);

    auto future = std::async(std::launch::async, [=] {
        device->cbf_async_indata_ind(pParams);
        PNIOD_indata_async_rsp(pParams);
    });
}

void PNIOD_cbf_sync_disconnect_ind(PNIOD_CBF_SYNC_DISCONNECT_IND_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;
}

void PNIOD_cbf_sync_data_status_ind(PNIOD_CBF_SYNC_DATA_STATUS_IND_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;
}

void PNIOD_cbf_async_prm_end_ind(PNIOD_CBF_ASYNC_PRM_END_IND_PARAMS_TYPE *pParams)
{
    qDebug() << QString("> Session %1: All submodules have been parameterized (%2).").arg(pParams->SessionKey).arg(pParams->ArContext);

    auto future = std::async(std::launch::async, [=]{
        device->cbf_async_prm_end_ind(pParams);
        PNIOD_prm_end_async_rsp(pParams);
    });
}

void PNIOD_cbf_sync_device_stopped(PNIOD_CBF_SYNC_STOPPED_PARAMS_TYPE *pParams)
{
    qDebug() << "> Device has been stopped.";

    if (device)
        device->cbf_sync_device_stopped(pParams);
}

void PNIOD_cbf_async_irt_init_inputs(PNIOD_CBF_ASYNC_IRT_INIT_INPUTS_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;

    auto future = std::async(std::launch::async, [=] {
        PNIOD_irt_init_inputs_async_rsp(pParams);
    });
}

/* PNIOD_CBF_SYNC_CP_STOP_REQ: reboot request */
void PNIOD_cbf_sync_cp_stop_req(PNIOD_CBF_SYNC_CP_STOP_REQ_PARAMS_TYPE *pParams)
{
    qDebug() << "> Device has received a stop request.";
    //shutdownDevice();
    //startupDevice();
}

void PNIOD_cbf_sync_start_led_flash(PNIOD_CBF_SYNC_START_LED_FLASH_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;
}

void PNIOD_cbf_sync_stop_led_flash(PNIOD_CBF_SYNC_STOP_LED_FLASH_PARAMS_TYPE *pParams)
{
    qDebug() << __FUNCTION__;
}

#endif