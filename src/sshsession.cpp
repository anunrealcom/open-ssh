#include "sshsession.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryFile>

SshSession::SshSession(QObject *parent)
    : ConnSession(parent)
{
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &SshSession::onReadyRead);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SshSession::onFinished);
    connect(m_proc, &QProcess::errorOccurred, this, &SshSession::onError);
}

SshSession::~SshSession()
{
    disconnect();
    cleanupAskpass();
}

bool SshSession::isConnected() const
{
    return m_proc && m_proc->state() == QProcess::Running;
}

static QString tempDir()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (d.isEmpty())
        d = QDir::currentPath();
    return d;
}

void SshSession::setupAskpass()
{
    cleanupAskpass();
    if (m_session.password.isEmpty())
        return;

    // 为避免批处理转义问题，把密码写入一个临时文件，助手脚本只负责 type 出来。
    QString base = QString("xbop_ask_%1").arg(QCoreApplication::applicationPid());
    m_askpassPass = tempDir() + "/" + base + ".pass";
    m_askpassScript = tempDir() + "/" + base + ".cmd";

    QFile passFile(m_askpassPass);
    if (!passFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    passFile.write(m_session.password.toUtf8());
    passFile.close();
    passFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    // Windows cmd 助手脚本：切换 UTF-8 码页后输出密码（无换行）
    QFile scriptFile(m_askpassScript);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QByteArray script;
    script += "@echo off\r\n";
    script += "chcp 65001 >nul\r\n";
    script += "type \"" + QDir::toNativeSeparators(m_askpassPass).toUtf8() + "\"\r\n";
    scriptFile.write(script);
    scriptFile.close();
    scriptFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    m_usingAskpass = true;
}

void SshSession::cleanupAskpass()
{
    if (!m_askpassPass.isEmpty()) {
        QFile::remove(m_askpassPass);
        m_askpassPass.clear();
    }
    if (!m_askpassScript.isEmpty()) {
        QFile::remove(m_askpassScript);
        m_askpassScript.clear();
    }
    m_usingAskpass = false;
}

void SshSession::connectTo(const Session &s)
{
    m_session = s;
    m_authSent = false;
    m_hostKeySent = false;
    m_connectedEmitted = false;

    // 准备 SSH_ASKPASS 密码助手
    setupAskpass();

    QStringList args;
    args << "-p" << QString::number(s.port);
    args << "-o" << "StrictHostKeyChecking=no";
    args << "-o" << "UserKnownHostsFile=/dev/null";
    args << "-o" << "PreferredAuthentications=password,keyboard-interactive,publickey";
    args << "-o" << "LogLevel=ERROR";
    args << "-o" << "NumberOfPasswordPrompts=3";
    if (!s.keyFile.isEmpty())
        args << "-i" << s.keyFile;
    // 强制分配伪终端，保证交互式 shell 正常工作（vim/top 等）
    args << "-tt";
    args << QString("%1@%2").arg(s.user, s.host);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (m_usingAskpass) {
        env.insert("SSH_ASKPASS", QDir::toNativeSeparators(m_askpassScript));
        env.insert("SSH_ASKPASS_REQUIRE", "force");
        env.insert("DISPLAY", "dummy");
    }
    m_proc->setProcessEnvironment(env);
    m_proc->start("ssh", args);
}

void SshSession::write(const QByteArray &data)
{
    if (isConnected())
        m_proc->write(data);
}

void SshSession::closeConn()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->write("\x03"); // Ctrl-C 友好的断开前清理
        m_proc->terminate();
        if (!m_proc->waitForFinished(1000))
            m_proc->kill();
    }
    emit disconnected();
    cleanupAskpass();
}

void SshSession::onReadyRead()
{
    QByteArray data = m_proc->readAllStandardOutput();
    if (data.isEmpty())
        return;

    // 自动确认未知主机密钥（仅在未使用 askpass 或 askpass 不支持时兜底）
    if (!m_hostKeySent &&
        (data.contains("yes/no") || data.contains("fingerprint"))) {
        m_proc->write("yes\n");
        m_hostKeySent = true;
    }

    // 密码处理：
    // 1) 优先使用 SSH_ASKPASS。若仍看到 "assword:"，说明 askpass 未生效，降级为手动输入。
    // 2) 未使用 askpass 时，按传统方式检测 prompt。
    if (!m_authSent && !m_session.password.isEmpty() &&
        data.contains("assword:")) {
        m_proc->write(m_session.password.toUtf8() + "\n");
        m_authSent = true;
    }

    if (!m_connectedEmitted) {
        m_connectedEmitted = true;
        emit connected();
    }

    emit output(data);
}

void SshSession::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    cleanupAskpass();
    emit disconnected();
}

void SshSession::onError(QProcess::ProcessError err)
{
    cleanupAskpass();
    QString msg;
    switch (err) {
    case QProcess::FailedToStart:
        msg = "无法启动 ssh，请确认系统已安装 OpenSSH 客户端（Windows 10+ 自带 ssh.exe）。";
        break;
    case QProcess::Crashed:
        msg = "ssh 进程已崩溃。";
        break;
    case QProcess::Timedout:
        msg = "ssh 操作超时。";
        break;
    default:
        msg = "ssh 连接错误。";
        break;
    }
    emit errorOccurred(msg);
}
