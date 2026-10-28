#include "tunnel.h"

#include <QCoreApplication>

Tunnel::Tunnel(QObject *parent)
    : QObject(parent)
{
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &Tunnel::onReadyRead);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Tunnel::onFinished);
    connect(m_proc, &QProcess::errorOccurred, this, &Tunnel::onError);
}

void Tunnel::start(const Session &s, Type type,
                   const QString &localAddr, int localPort,
                   const QString &remoteHost, int remotePort)
{
    m_session = s;
    m_hostKeySent = false;
    m_authSent = false;
    m_started = false;

    QString bind = localAddr.isEmpty() ? "127.0.0.1" : localAddr;
    QString fwd;
    if (type == Local) {
        // ssh -L [bind:]localPort:remoteHost:remotePort
        fwd = QString("%1:%2:%3:%4").arg(bind).arg(localPort).arg(remoteHost).arg(remotePort);
    } else {
        // ssh -R [bind:]remotePort:localHost:localPort
        fwd = QString("%1:%2:%3:%4").arg(remotePort).arg(bind).arg(localPort);
    }

    QStringList args;
    args << (type == Local ? "-L" : "-R") << fwd;
    args << "-N"; // 不执行远程命令，仅转发
    args << "-p" << QString::number(s.port);
    args << "-o" << "StrictHostKeyChecking=no";
    args << "-o" << "UserKnownHostsFile=/dev/null";
    args << "-o" << "PreferredAuthentications=password,keyboard-interactive,publickey";
    args << "-o" << "LogLevel=ERROR";
    if (!s.keyFile.isEmpty())
        args << "-i" << s.keyFile;
    args << "-tt"; // 强制 PTY 以便自动应答密码
    args << QString("%1@%2").arg(s.user, s.host);

    m_desc = QString("[%1] %2 -> %3@%4 (%5:%6)")
                 .arg(type == Local ? "L" : "R")
                 .arg(type == Local
                          ? QString("%1:%2").arg(bind).arg(localPort)
                          : QString("%1:%2").arg(bind).arg(localPort))
                 .arg(s.user, s.host)
                 .arg(remoteHost).arg(remotePort);

    m_proc->start("ssh", args);
}

void Tunnel::stop()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->terminate();
        if (!m_proc->waitForFinished(1000))
            m_proc->kill();
    }
    emit stopped();
}

bool Tunnel::isRunning() const
{
    return m_proc && m_proc->state() == QProcess::Running;
}

QString Tunnel::description() const
{
    return m_desc;
}

void Tunnel::onReadyRead()
{
    QByteArray data = m_proc->readAllStandardOutput();
    if (data.isEmpty())
        return;

    if (!m_hostKeySent && (data.contains("yes/no") || data.contains("fingerprint"))) {
        m_proc->write("yes\n");
        m_hostKeySent = true;
    }
    if (!m_authSent && !m_session.password.isEmpty() && data.contains("assword:")) {
        m_proc->write(m_session.password.toUtf8() + "\n");
        m_authSent = true;
    }
    if (!m_started) {
        m_started = true;
        emit started();
    }
}

void Tunnel::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    m_started = false;
    emit stopped();
}

void Tunnel::onError(QProcess::ProcessError err)
{
    QString msg;
    switch (err) {
    case QProcess::FailedToStart:
        msg = "无法启动 ssh，请确认系统已安装 OpenSSH 客户端。";
        break;
    case QProcess::Crashed:
        msg = "隧道进程已退出。";
        break;
    default:
        msg = "隧道错误。";
        break;
    }
    emit errorOccurred(msg);
}
