#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QThread>

#include "conn.h"

class QThread;

/**
 * 串口读线程：在子线程阻塞读取串口数据，读到即通过信号抛出。
 */
class SerialReader : public QThread
{
    Q_OBJECT
public:
    explicit SerialReader(void *handle, QObject *parent = nullptr);
signals:
    void received(const QByteArray &data);
protected:
    void run() override;
private:
    void *m_handle;
};

/**
 * 串口会话：基于 Windows 原生 API（CreateFile / DCB / ReadFile / WriteFile）。
 * 本机 Qt 未编译 SerialPort 模块，故不依赖 QtSerialPort，保证可本地编译。
 * 仅支持 Windows（单机工具目标平台）。
 */
class SerialSession : public ConnSession
{
    Q_OBJECT
public:
    struct Config
    {
        QString port = "COM3";   // 端口名，如 COM3
        int baudRate = 115200;
        int dataBits = 8;
        int stopBits = 1;        // 1 / 1.5 / 2
        char parity = 'N';       // N / O / E
    };

    explicit SerialSession(QObject *parent = nullptr);
    ~SerialSession();

    void start(const Config &cfg);
    bool isOpen() const;

public slots:
    void write(const QByteArray &data) override;
    void closeConn() override;

signals:
    void dataReady(const QByteArray &data); // 内部：读线程 -> 主线程

private slots:
    void onDataReady(const QByteArray &data);

private:
    Config m_cfg;
    void *m_handle = nullptr;     // HANDLE
    SerialReader *m_reader = nullptr;
    volatile bool m_closing = false;
};
