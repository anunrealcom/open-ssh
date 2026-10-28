#pragma once

#include <QObject>
#include <QProcess>

#include "session.h"

/**
 * SSH 端口转发（隧道）：本地转发 -L 或远程转发 -R。
 * 复用系统 OpenSSH，配合 -N（不执行远程命令）长期驻留。
 * 为支持密码认证，强制分配伪终端 -tt，并自动应答主机密钥与密码。
 */
class Tunnel : public QObject
{
    Q_OBJECT
public:
    enum Type { Local, Remote };

    explicit Tunnel(QObject *parent = nullptr);

    // 配置并启动隧道
    // localAddr : 本地绑定地址（默认 127.0.0.1）
    // localPort : 本地监听端口
    // remoteHost: 转发目标主机
    // remotePort: 转发目标端口
    void start(const Session &s, Type type,
               const QString &localAddr, int localPort,
               const QString &remoteHost, int remotePort);
    void stop();
    bool isRunning() const;
    QString description() const;

signals:
    void started();
    void stopped();
    void errorOccurred(const QString &msg);

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onError(QProcess::ProcessError err);

private:
    QProcess *m_proc = nullptr;
    Session m_session;
    QString m_desc;
    bool m_hostKeySent = false;
    bool m_authSent = false;
    bool m_started = false;
};
