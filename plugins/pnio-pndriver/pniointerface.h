#ifndef PNIOINTERFACE_H
#define PNIOINTERFACE_H

#include <memory>

#include <QtCore>

#ifndef Q_MOC_RUN
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#endif
using namespace boost::accumulators;

#ifdef Q_OS_WIN
#pragma warning(push)
#pragma warning(disable: 4005)
#include <windows.h>
#pragma warning(pop)
#endif

#include "pniobase.h"

#include <logic.h>
#include <interfaces.h>
#include <iointerface.h>
#include <pniovalue.h>

#ifdef WIN32
    #define DEBUG_LOOP  0
#else
    #define DEBUG_LOOP  1
#endif


Q_DECLARE_METATYPE(PNIO_IOXS)
Q_DECLARE_METATYPE(PNIO_UINT32)

typedef struct {
    PNIO_UINT32 slot;
    PNIO_UINT32 subslot;
    PNIO_IO_TYPE io_type;
    PNIO_UINT32 data_length;
	PNIO_UINT32 hw_id;

    PNIO_UINT16 station_number;
    PNIO_UINT16 hardware_slot;

    PNIO_UINT32 mod_id;
    PNIO_UINT32 subslot_id;
    PNIO_UINT32 api;
    PNIO_UINT32 max_subslots;
    PNIO_UINT32 mod_state;
    PNIO_UINT32 sub_state;
} PNIO_MODULE;


// callbacks
void callback_for_irt(PNIO_CP_CBE_PRM * pCbfPrm);
void callback_for_irt_cycle(PNIO_CP_CBE_PRM * pCbfPrm);

class PNIOInterface : public IOInterface
{
    Q_OBJECT
public:
    explicit PNIOInterface(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, 
        const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config, 
        const std::vector<std::shared_ptr<PNIORecord>> &record_config);

    virtual void updateConfig(const std::vector<std::shared_ptr<PNIOInputValue>> &in_config, 
        const std::vector<std::shared_ptr<PNIOOutputValue>> &out_config, 
        const std::vector<std::shared_ptr<PNIORecord>> &record_config);
    std::vector<std::shared_ptr<PNIOInputValue>> getInConfig() const { return _in_config; }
    std::vector<std::shared_ptr<PNIOOutputValue>> getOutConfig() const { return _out_config; }
	std::vector<std::shared_ptr<PNIOWriteRecord>> getWriteRecordConfig() const { return _write_record_config; }
	std::vector<std::shared_ptr<PNIOReadRecord>> getReadRecordConfig() const { return _read_record_config; }

    void setLogic(std::shared_ptr<Logic> logic);
    void writeOutputs() override;
    
    QList<PluginInterface::Message> alarmMessages() const;

    bool hasError() const;
    virtual bool isInitialized() const = 0;

	// helper
    static const QString errorDescription(const PNIO_UINT32 errorCode);

signals:
    // CommunicationInterface
    void valueChanged(const QString &name, const QVariant &value);

public slots:
    // triggers async fetching of data. called by timer.
    void readData();

    // slots for helper
    void irtCycle(const PNIO_UINT32 handle);

	virtual void connectPNIO() = 0;
    virtual void disconnectPNIO() = 0;

    void printStatistics();

    void currentMachineState(const PluginInterface::MachineState machineState, const PluginInterface::DiagState diagState) { _current_state = machineState; }

protected:
    PNIO_UINT32 _handle;   // handle for interface
	bool _ready;

    //device errors
    enum DeviceError : int {
        noErr = 0,
        connectionLost = 1 << 1,
        moduleUnplugged = 1 << 2,
        moduleReadErr = 1 << 3,
        moduleWriteErr = 1 << 4
    };
    std::map<std::pair<PNIO_UINT16, PNIO_UINT16>, PNIO_UINT32> _module_errors; // StatNo , Slot , error mask

    // config
    std::vector<PNIO_MODULE> _config;
    std::vector<std::shared_ptr<PNIOInputValue>> _in_config;
    std::vector<std::shared_ptr<PNIOOutputValue>> _out_config;
	std::vector<std::shared_ptr<PNIOWriteRecord>> _write_record_config;
	std::vector<std::shared_ptr<PNIOReadRecord>> _read_record_config;


	std::map<PNIO_UINT32, PNIO_UINT32> _in_slotToHardwareId;

    // data
    std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> _out_data;
    
	// initialize buffers
	void prepareBuffer();

    // protected functions for connecting and disconnecting
    virtual PNIO_UINT32 initialize() = 0;
    virtual PNIO_UINT32 uninitialize() = 0;

	PNIO_UINT32 registerIRT();

	void startTimer();
	void stopTimer();

    // convert raw data to name/value pairs
    void processData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data, const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> &state);

    // triggers async reading and writing of data to the controller.
    virtual void readDataInternal() = 0;
    virtual std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, PNIO_IOXS> writeData(const std::map<std::pair<PNIO_UINT32, PNIO_UINT32>, std::vector<std::byte>> &data) = 0;

private:
    PluginInterface::MachineState _current_state = PluginInterface::MachineState::Off;

    std::weak_ptr<Logic> _logic;

    QMutex _timerMutex;

	QTimer *_read_timer;

	QElapsedTimer _timer;
    accumulator_set<int64_t, features<tag::min, tag::max, tag::mean, tag::variance > > _read_jitter;
    accumulator_set<int64_t, features<tag::min, tag::max, tag::mean, tag::variance > > _read_performance;

	int64_t _min_jitter;
	int64_t _mean_jitter;
	int64_t _max_jitter;
    int64_t _variance_jitter;

	int64_t _min_performance;
	int64_t _mean_performance;
	int64_t _max_performance;
    int64_t _variance_performance;
};

#endif // PNIOINTERFACE_H
