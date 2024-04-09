#ifndef BPNSERIALPORT_H
#define BPNSERIALPORT_H

#include <logic.h>
#include <pniovalue.h>

#include <QApplication>
#include <QDeadlineTimer>
#include <QIODevice>
#include <QMutex>
#include <QReadWriteLock>
#include <QSemaphore>
#include <QThread>

class BPNSerialPort : public QIODevice
{
    Q_OBJECT

  public:
    BPNSerialPort(const uint32_t slot, QObject* parent = nullptr);
    ~BPNSerialPort();

    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    void clear();
    bool isSequential() const override;
    bool open(QIODevice::OpenMode mode) override;
    QHash<QString, QVariant> profinetConfig();
    bool waitForReadyRead(int msecs) override;

  signals:
    void valueChanged(const QString& name, const QVariant& value);

  public slots:
    void setValues(const QHash<QString, QVariant>& values);

  private slots:
    void checkSendStatus();
    void checkRCVStatus();

  private:
    // functions
    qint64 readData(char* data, qint64 maxSize) override;
    bool saveNewData(std::vector<std::byte> bytes);
    qint64 writeData(const char* data, qint64 maxSize) override;
    void resetSequenznumberToLastValidValue();

    // variablen
    uint32_t _slot;

    QByteArray _data;
    bool _firstReadRecord = true;
    bool _isConnected = false;
    QReadWriteLock _readWriteLock;
    uint8_t _sequenznumber = 0;
    uint8_t _lastSequenznumberRCV = 0;

    QSemaphore _sem_write;
    QSemaphore _sem_write_req;
    QSemaphore _sem_ready_read;
    QSemaphore _sem_ready_read_req;

    QThread* _t_SendStatus;
    QThread* _t_RCVStatus;

    // PNIO
    input<bool> _DTR_Status;
    input<bool> _DSR_Status;
    input<bool> _RTS_Status;
    input<bool> _CTS_Status;
    input<bool> _DCD_Status;
    input<bool> _Ring_Status;
    input<bool> _Reserviert;
    input<bool> _SGN_STAT;

    input<uint8_t> _SEND_Receipt = {255};
    input<uint16_t> _SEND_Status;
    input<uint8_t> _sequenznumber_RCV;

    WriteRecord _record_write;
    WriteRecord _record_clear;
    ReadRecord _record_read1;
    ReadRecord _record_read2;
};

#endif  // BPNSERIALPORT_H
