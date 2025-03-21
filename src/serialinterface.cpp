/*******************************************************************************
 * File:        serialinterface.cpp
 * Created:     23 June 2020
 * Author:      Timo Hueser
 * Contact:     timo.hueser@gmail.com
 * Copyright:   2021 Timo Hueser
 * License:     LGPL v3.0
 ******************************************************************************/

#include "serialinterface.hpp"

QList<QString> SerialInterface::getAvailableDevices() {
    QList<QSerialPortInfo> serialPortInfos = QSerialPortInfo::availablePorts();
    QList<QString> deviceNames;
    for (const auto &port : serialPortInfos) {
        deviceNames.append(port.description());
    }
    return deviceNames;
}

SerialInterface::SerialInterface(const QString &deviceName, QObject *parent) {
    QList<QSerialPortInfo> serialPortInfos = QSerialPortInfo::availablePorts();
    for (int i = 0; i < serialPortInfos.count(); i++) {
        if (serialPortInfos[i].description() == deviceName) {
            serialPortName = serialPortInfos[i].portName();
            qDebug() << serialPortName;
            serialPort = new QSerialPort(parent);
            serialPort->setPortName(serialPortName);
            if (serialPort->open(QIODevice::ReadWrite)) {
                serial_conn = true;
                serialPort->setBaudRate(QSerialPort::Baud115200);
                serialPort->setParity(QSerialPort::NoParity);
                serialPort->setStopBits(QSerialPort::OneStop);
                serialPort->setFlowControl(QSerialPort::NoFlowControl);
                serialPort->setDataBits(QSerialPort::Data8);
                delayl(1000);
            } else {
                return;
            }
        }
    }

    connect(serialPort, &QSerialPort::readyRead, [&]() {
        // this is called when readyRead() is emitted
        // QByteArray datas = serialPort->readAll();
        emit serialReadReadySignal();
        // TODO consider adding a ring buffer to this class
    });
    connect(serialPort, &QSerialPort::errorOccurred,
            [&](QSerialPort::SerialPortError error) {
                // this is called when a serial communication error
                // occurs

                qDebug() << "An error occured: " << error;
                // return qApp->quit();
            });
}

SerialInterface::~SerialInterface() { serialPort->close(); }

int SerialInterface::write(int val) {
    QByteArray cmd;
    cmd[0] = val >> 8;
    cmd[1] = val;
    if (serial_conn) {
        serialPort->write(cmd);
        serialPort->waitForBytesWritten(1);
        return 1;
    }
    return 0;
}

int SerialInterface::writeBuffer(const char *buffer, unsigned int len) {
    QByteArray cmd;
    if (serial_conn) {
        serialPort->write(buffer, len);
        serialPort->waitForBytesWritten(len);
        return 1;
    }
    return 0;
}

int SerialInterface::send_instruction(int mode, int readwrite, int val1,
                                      int val2) {
    QByteArray cmd;
    cmd[0] = mode;
    cmd[1] = readwrite;
    cmd[2] = val1 >> 8;
    cmd[3] = val1;
    cmd[4] = val2 >> 8;
    cmd[5] = val2;
    if (serial_conn) {
        serialPort->write(cmd);
        serialPort->waitForBytesWritten(1);
    } else
        return 0;
    return 1;
}

unsigned int SerialInterface::get_cobs_packet(char *buffer, unsigned int len,
                                              int msec) {
    static char temp[256]; // Max size COBS
    static int curser = 0;
    int msg_len;

    for (curser; curser <= std::min((unsigned int)sizeof(temp) - 1, len);
         curser++) {
        if (!serialPort->bytesAvailable()) {
            return 0;
        }
        serialPort->read((char *)&(temp[curser]), 1);
        if (temp[curser] == 0) {
            msg_len = curser + 1;
            curser = 0;
            break;
        }
    }
    memcpy((void *)buffer, (void *)temp, msg_len);
    return msg_len;
}

int SerialInterface::get_answer(int answer[]) {
    uint8_t temp[5];
    temp[0] = 0;
    for (int i = 0; i < 5; i++) {
        serialPort->waitForReadyRead(10);
    }
    int nread = serialPort->read((char *)temp, 5);
    if (nread != 5) {
        qDebug() << "Error Reading Answer";
        return 0;
    }
    answer[0] = ((int8_t)temp[1] << 8) + temp[2];
    answer[1] = ((int8_t)temp[3] << 8) + temp[4];
    return (int)(temp[0]);
}

int SerialInterface::get_answer() {
    uint8_t temp[5];
    temp[0] = 0;
    for (int i = 0; i < 5; i++) {
        serialPort->waitForReadyRead(10);
    }
    int nread = serialPort->read((char *)temp, 5);
    if (nread != 5) {
        qDebug() << "Error Reading Answer";
        return 0;
    }
    return (int)(temp[0]);
}
