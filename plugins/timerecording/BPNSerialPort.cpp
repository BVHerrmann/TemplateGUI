#include "BPNSerialPort.h"

BPNSerialPort::BPNSerialPort(const uint32_t slot, QObject* parent) : QIODevice(parent)
{
    _slot = slot;

    _t_SendStatus = new QThread();
    auto* sendTimer = new QTimer(nullptr);
    sendTimer->setInterval(10);
    sendTimer->moveToThread(_t_SendStatus);
    connect(sendTimer, &QTimer::timeout, this, &BPNSerialPort::checkSendStatus, Qt::DirectConnection);
    sendTimer->connect(_t_SendStatus, &QThread::started, sendTimer, qOverload<>(&QTimer::start));

    _t_RCVStatus = new QThread();
    auto* rcvTimer = new QTimer(nullptr);
    rcvTimer->setInterval(10);
    rcvTimer->moveToThread(_t_RCVStatus);
    connect(rcvTimer, &QTimer::timeout, this, &BPNSerialPort::checkRCVStatus, Qt::DirectConnection);
    rcvTimer->connect(_t_RCVStatus, &QThread::started, rcvTimer, qOverload<>(&QTimer::start));
}

BPNSerialPort::~BPNSerialPort()
{
}

bool BPNSerialPort::atEnd() const
{
    return (bytesAvailable() == 0);
}

bool BPNSerialPort::open(QIODevice::OpenMode mode)
{
    if (_isConnected) {
        return QIODevice::open(mode);
    }

    return false;
}

qint64 BPNSerialPort::bytesAvailable() const
{
    return _data.size() + QIODevice::bytesAvailable();
}

bool BPNSerialPort::isSequential() const
{
    return true;
}

bool BPNSerialPort::waitForReadyRead(int msecs)
{
    _readWriteLock.lockForRead();

    if (_data.size() > 0) {
        _readWriteLock.unlock();
        return true;
    }

    _sem_ready_read_req.release();
    _readWriteLock.unlock();

    const bool acquired = _sem_ready_read.tryAcquire(1, msecs);
    if (!acquired) {
        _readWriteLock.lockForRead();
        if (_sem_ready_read_req.available() > 0) {
            _sem_ready_read_req.acquire();
        }
        _readWriteLock.unlock();
    }

    return acquired;
}

QHash<QString, QVariant> BPNSerialPort::profinetConfig()
{
    QHash<QString, QVariant> config;

    QVector<std::shared_ptr<PNIOInputValue>> in_config;
    in_config << std::make_shared<PNIOInputValue>(_DTR_Status, tr("Data terminal ready"), _slot, 0, 0);
    in_config << std::make_shared<PNIOInputValue>(_DSR_Status, tr("Data set ready"), _slot, 0, 1);
    in_config << std::make_shared<PNIOInputValue>(_RTS_Status, tr("Request to send"), _slot, 0, 2);
    in_config << std::make_shared<PNIOInputValue>(_CTS_Status, tr("Clear to send"), _slot, 0, 3);
    in_config << std::make_shared<PNIOInputValue>(_DCD_Status, tr("Data Carrier detect"), _slot, 0, 4);
    in_config << std::make_shared<PNIOInputValue>(_Ring_Status, tr("Ring Indicator"), _slot, 0, 5);
    in_config << std::make_shared<PNIOInputValue>(_Reserviert, tr("Reserved"), _slot, 0, 6);
    in_config << std::make_shared<PNIOInputValue>(_SGN_STAT, tr("RS232 secondary signals available"), _slot, 0, 7);
    in_config << std::make_shared<PNIOInputValue>(_SEND_Receipt, tr("Send Receipt"), _slot, 1);
    in_config << std::make_shared<PNIOInputValue>(_SEND_Status, tr("Send Status"), _slot, 2);
    in_config << std::make_shared<PNIOInputValue>(_sequenznumber_RCV, tr("Data terminal ready"), _slot, 4);
    config["in_config"] = QVariant::fromValue(in_config);

    QVector<std::shared_ptr<PNIORecord>> record_config;
    record_config << std::make_shared<PNIOWriteRecord>(_record_write, "Write", _slot, 0x30);
    record_config << std::make_shared<PNIOReadRecord>(_record_read1, "Read1", _slot, 0x31);
    record_config << std::make_shared<PNIOReadRecord>(_record_read2, "Read2", _slot, 0x32);
    record_config << std::make_shared<PNIOWriteRecord>(_record_clear, "Clear", _slot, 0x36);
    config["record_config"] = QVariant::fromValue(record_config);

    return config;
}

qint64 BPNSerialPort::readData(char* data, qint64 maxSize)
{
    QWriteLocker locker(&_readWriteLock);
    if (maxSize == 0) {
        return 0;
    }

    if (maxSize && _data.size() > 0 && data != nullptr) {
        const int size = _data.size() <= maxSize ? _data.size() : maxSize;
        memcpy(data, _data.constData(), size);
        _data.remove(0, size);

        return size;
    }

    return -1;
}

void BPNSerialPort::checkSendStatus()
{
    const auto send_status = _SEND_Status.get();

    if (send_status != 0 || _SEND_Receipt.get() == _sequenznumber) {
        if (_sem_write_req.tryAcquire()) {
            const bool is_data_set_transer_in_progess = send_status == 0x7001 || send_status == 0x7002;
            if (is_data_set_transer_in_progess) {
                _sem_write_req.release();
                return;
            }

            if (send_status == 0x81D7) {
                resetSequenznumberToLastValidValue();
            }
            if (send_status != 0) {
                qDebug() << "Send Status:" << send_status;
            }
            _sem_write.release();
        }
    }
}

void BPNSerialPort::checkRCVStatus()
{
    const uint8_t lastSequenznumberRCV = _sequenznumber_RCV.get();
    if (lastSequenznumberRCV != _lastSequenznumberRCV) {
        const bool isDataValid = saveNewData(_firstReadRecord ? _record_read1.read() : _record_read2.read());
        _firstReadRecord = !_firstReadRecord;

        if (isDataValid) {
            _lastSequenznumberRCV = lastSequenznumberRCV;
        }
    }
}

qint64 BPNSerialPort::writeData(const char* data, qint64 maxSize)
{
    QByteArray byteArray(maxSize, 0);
    memcpy(byteArray.data(), data, maxSize);

    const int maxByteLengthOfTelegramm = 230;

    if (byteArray.size() < maxByteLengthOfTelegramm) {
        _sequenznumber = (_sequenznumber % 255) + 1;

        byteArray.prepend(1);
        byteArray.prepend(_sequenznumber);
    } else {
        abort();
    }

    std::vector<std::byte> bytes;
    bytes.resize(byteArray.size());
    memcpy(bytes.data(), byteArray.data(), byteArray.size());

    const bool result = _record_write.write(bytes);
    if (!result) {
        return -1;
    }

    _sem_write_req.release();
    _sem_write.acquire();

    int size = maxSize;
    const uint16_t send_status = _SEND_Status.get();

    /*   if (!(send_status == 0x7000 || send_status == 0x7001 || send_status == 0x7002)) {*/
    if (_SEND_Receipt.get() != _sequenznumber || send_status != 0) {
        size = -1;
    }

    emit bytesWritten(size);
    return size;
}

void BPNSerialPort::resetSequenznumberToLastValidValue()
{
    _sequenznumber = _SEND_Receipt.get();
}

void BPNSerialPort::clear()
{
    std::vector<std::byte> data{(std::byte)0x00, (std::byte)0x36, (std::byte)0x00, (std::byte)0x08, (std::byte)0x01, (std::byte)0x00,
                                (std::byte)0x00, (std::byte)0x00, (std::byte)0x01, (std::byte)0x00, (std::byte)0x00, (std::byte)0x00};

    uint32_t record_index = 0x36;

    _record_clear.write(data);
}

bool BPNSerialPort::saveNewData(std::vector<std::byte> bytes)
{
    QWriteLocker locker(&_readWriteLock);

    QByteArray data(bytes.size(), 0);
    memcpy(data.data(), bytes.data(), bytes.size());

    if (data.size() >= 4) {
        QByteArray header = data.left(4);
        if (header.at(2) == 0) {
            if (data.size() > 4) {
                _data.append(data.mid(4));
                if (_sem_ready_read_req.tryAcquire()) {
                    _sem_ready_read.release();
                }
                emit readyRead();
                return true;
            }
        }
    }

    return false;
}

void BPNSerialPort::setValues(const QHash<QString, QVariant>& values)
{
    QHash<QString, QVariant>::const_iterator i = values.constBegin();
    while (i != values.constEnd()) {
        if (i.key() == "PNIO/controller_connected") {
            _isConnected = true;
            _t_SendStatus->start();
            _t_RCVStatus->start();
        } else if (i.key() == "PNIO/controller_disconnected") {
            _isConnected = false;
        }
        ++i;
    }
}