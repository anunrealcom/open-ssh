#pragma once

#include <QObject>
#include <QProcess>

#include "session.h"
#include "conn.h"

/**
 * 基于系统 OpenSSH（ssh.exe / ssh）的交互式会话。
 * 通过 QProcess 启动 ssh，并合并 stdout/stderr 通道，
 * 监听 "assword:" 自动发送密码、监听 "(yes/no)" 自动确认主机密钥。
 */
class SshSession : public ConnSession
{
    Q_OBJECT
public:
    explicit SshSession(QObject *parent = nullptr);
    ~SshSession();

    void connectTo(const Session &s);
    bool isConnected() const;

public slots:
    void write(const QByteArray &data) override;
    void closeConn() override;

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onError(QProcess::ProcessError err);

private:
    void setupAskpass();
    void cleanupAskpass();

    QProcess *m_proc = nullptr;
    Session m_session;
    bool m_authSent = false;
    bool m_hostKeySent = false;
    bool m_connectedEmitted = false;

    // SSH_ASKPASS 临时文件（Windows .cmd + 密码文件）
    QString m_askpassScript;
    QString m_askpassPass;
    bool m_usingAskpass = false;
};
