#include "serialsession.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

SerialReader::SerialReader(void *handle, QObject *parent)
    : QThread(parent), m_handle(handle)
{
}

void SerialReader::run()
{
    HANDLE h = static_cast<HANDLE>(m_handle);
    BYTE buf[256];
    DWORD n = 0;
    // 设置读超时，使阻塞 ReadFile 最多等待 200ms，便于线程退出
    COMMTIMEOUTS ct;
    ZeroMemory(&ct, sizeof(ct));
    ct.ReadIntervalTimeout = MAXDWORD;
    ct.ReadTotalTimeoutConstant = 200;
    ct.ReadTotalTimeoutMultiplier = 0;
    SetCommTimeouts(h, &ct);

    while (!isInterruptionRequested()) {
        if (ReadFile(h, buf, sizeof(buf), &n, NULL) && n > 0) {
            emit received(QByteArray(reinterpret_cast<char *>(buf), static_cast<int>(n)));
        }
    }
}

SerialSession::SerialSession(QObject *parent)
    : ConnSession(parent), m_handle(nullptr)
{
}

SerialSession::~SerialSession()
{
    closeConn();
}

void SerialSession::start(const Config &cfg)
{
    m_cfg = cfg;
    // 构造 Windows 串口设备路径：\\.\COM3（COM10+ 必须带前缀）
    QString path = cfg.port;
    if (!path.startsWith("\\\\.\\"))
        path = "\\\\.\\" + path;
    QByteArray p = path.toLocal8Bit();
    HANDLE h = CreateFileA(p.constData(), GENERIC_READ | GENERIC_WRITE,
                           0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        emit errorOccurred(QString("无法打开串口 %1（可能被占用或无权限）。").arg(cfg.port));
        return;
    }
    m_handle = h;

    DCB dcb;
    ZeroMemory(&dcb, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        m_handle = nullptr;
        emit errorOccurred("无法读取串口配置。");
        return;
    }
    dcb.BaudRate = static_cast<DWORD>(cfg.baudRate);
    dcb.ByteSize = static_cast<BYTE>(cfg.dataBits);
    dcb.StopBits = cfg.stopBits == 2 ? TWOSTOPBITS
                                     : (cfg.stopBits == 1 ? ONESTOPBIT : ONE5STOPBITS);
    dcb.Parity = (cfg.parity == 'E') ? EVENPARITY
                                     : (cfg.parity == 'O' ? ODDPARITY : NOPARITY);
    dcb.fParity = (cfg.parity != 'N');
    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        m_handle = nullptr;
        emit errorOccurred("无法应用串口参数（波特率/数据位/停止位/校验）。");
        return;
    }
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    m_reader = new SerialReader(h, this);
    connect(m_reader, &SerialReader::received, this, &SerialSession::onDataReady);
    m_reader->start();

    emit connected();
}

void SerialSession::onDataReady(const QByteArray &data)
{
    emit output(data);
}

void SerialSession::write(const QByteArray &data)
{
    if (!m_handle)
        return;
    HANDLE h = static_cast<HANDLE>(m_handle);
    DWORD written = 0;
    WriteFile(h, data.constData(), static_cast<DWORD>(data.size()), &written, NULL);
}

void SerialSession::closeConn()
{
    m_closing = true;
    if (m_reader) {
        m_reader->requestInterruption();
        if (!m_reader->wait(1000))
            m_reader->terminate();
        m_reader->deleteLater();
        m_reader = nullptr;
    }
    if (m_handle) {
        CloseHandle(static_cast<HANDLE>(m_handle));
        m_handle = nullptr;
    }
    emit disconnected();
}

bool SerialSession::isOpen() const
{
    return m_handle != nullptr;
}
