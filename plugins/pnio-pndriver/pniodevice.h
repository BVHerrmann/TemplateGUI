#ifndef PLUGINS_PNIODEVICE_PNIODEVICE_H_
#define PLUGINS_PNIODEVICE_PNIODEVICE_H_

#include "pniointerface.h"

#include <QtCore>

#if 0

#include <pniousrd.h>

/***************************************************/
/* PNIO_ANNOTATION entries                         */
/***************************************************/

#define ANNOT_NAME      "BERTRAM"       /* device type (String 25) */
#define ANNOT_ORDERID   "9955"          /* Order Id    (String 20) */
#define ANNOT_HW_REV     0              /* HwRevision  (short)     */
#define ANNOT_SW_PREFIX  'V'            /* SwRevisionPrefix (char) */
#define ANNOT_SW_REV_1   0              /* SwRevision1 (short)     */
#define ANNOT_SW_REV_2   0              /* SwRevision2 (short)     */
#define ANNOT_SW_REV_3   0              /* SwRevision3 (short)     */


class PNIODevice : public PNIOInterface
{
    Q_OBJECT
public:
    explicit PNIODevice(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config);
    virtual ~PNIODevice();

    // forwarded callbacks for device
    PNIO_IOXS cbf_data_write(const PNIO_DEV_ADDR* pAddr, const PNIO_UINT32 BufLen, PNIO_UINT8* pBuffer, const PNIO_IOXS Iocs);
    PNIO_IOXS cbf_data_read(const PNIO_DEV_ADDR* pAddr, const PNIO_UINT32 BufLen, const PNIO_UINT8* pBuffer, const PNIO_IOXS Iops);
    void cbf_async_indata_ind(PNIOD_CBF_ASYNC_INDATA_IND_PARAMS_TYPE *pParams);
    void cbf_async_prm_end_ind(PNIOD_CBF_ASYNC_PRM_END_IND_PARAMS_TYPE *pParams);
    void cbf_sync_device_stopped(PNIOD_CBF_SYNC_STOPPED_PARAMS_TYPE *pParams);

signals:

public slots:
    void connectPNIO() override;
    void disconnectPNIO() override;

private:
    PNIO_UINT16 _session_key;     // session identifier, this is obtained in application-relation information callback i.e. PNIO_cbf_ar_info_ind.
    PNIO_UINT16 _ar_number;       // application relation number

    std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> _out_state;
    
    // protected functions for connecting and disconnecting
    PNIO_UINT32 initialize() override;
    PNIO_UINT32 uninitialize() override;

    void readConfig();
    
    PNIO_UINT32 startDevice();
    PNIO_UINT32 stopDevice();

	// helper
    QSemaphore _indata_ind_semaphore;
	QSemaphore _device_stopped_semaphore;

    // triggers async reading and writing of data to the controller.
	void readDataInternal() override;
    std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> writeData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data) override;
};

#endif

#endif  // PLUGINS_PNIODEVICE_PNIODEVICE_H_
