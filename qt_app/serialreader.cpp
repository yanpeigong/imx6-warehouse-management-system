#include "serialreader.h"
#include <QDebug>

SerialReader::SerialReader(QObject *parent) : QObject(parent) {
    connect(&port_, &QSerialPort::readyRead,
            this,   &SerialReader::onReadyRead);
    connect(&port_, &QSerialPort::errorOccurred,
            this,   &SerialReader::onError);
}

SerialReader::~SerialReader() {
    close();
}

bool SerialReader::open(const QString &portName, int baud) {
    if (port_.isOpen()) port_.close();
    port_.setPortName(portName);
    port_.setBaudRate(baud);
    port_.setDataBits(QSerialPort::Data8);
    port_.setParity(QSerialPort::NoParity);
    port_.setStopBits(QSerialPort::OneStop);
    port_.setFlowControl(QSerialPort::NoFlowControl);
    if (!port_.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("串口打开失败: %1").arg(port_.errorString()));
        return false;
    }
    qDebug() << "串口已打开:" << portName << " 波特率:" << baud;
    return true;
}

void SerialReader::close() {
    if (port_.isOpen()) port_.close();
}

void SerialReader::onReadyRead() {
    buffer_.append(port_.readAll());
    // 按 \r 或 \n 切分
    while (true) {
        int idx = -1;
        for (int i = 0; i < buffer_.size(); ++i) {
            if (buffer_[i] == '\r' || buffer_[i] == '\n') { idx = i; break; }
        }
        if (idx < 0) break;
        QByteArray one = buffer_.left(idx);
        buffer_.remove(0, idx + 1);
        if (!one.isEmpty()) {
            QString barcode = QString::fromUtf8(one).trimmed();
            if (!barcode.isEmpty()) {
                qDebug() << "扫到条码:" << barcode;
                emit barcodeScanned(barcode);
            }
        }
    }
}

void SerialReader::onError(QSerialPort::SerialPortError err) {
    if (err == QSerialPort::NoError) return;
    emit errorOccurred(port_.errorString());
}
