#ifndef SERIALREADER_H
#define SERIALREADER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>

/**
 * 串口监听类
 * - 持续监听串口数据
 * - 以 \r 或 \n 作为一个条码的结束符
 * - 拼接到完整条码后，通过 barcodeScanned 信号发出
 *
 * 大多数扫码枪默认在条码末尾追加 \r\n，所以以换行作分隔是稳妥的
 */
class SerialReader : public QObject {
    Q_OBJECT
public:
    explicit SerialReader(QObject *parent = nullptr);
    ~SerialReader();

    // 打开串口；常见参数：portName="/dev/ttyUSB0", baud=9600
    bool open(const QString &portName, int baud = 9600);
    void close();
    bool isOpen() const { return port_.isOpen(); }

signals:
    // 扫到一个完整条码时发出
    void barcodeScanned(const QString &barcode);
    // 串口出错
    void errorOccurred(const QString &msg);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError err);

private:
    QSerialPort port_;
    QByteArray  buffer_;   // 缓存未结束的字节
};

#endif // SERIALREADER_H
