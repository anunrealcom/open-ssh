#include "filetransfer.h"
#include "xbmessagebox.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>

FileTransfer::FileTransfer(QObject *parent)
    : QObject(parent)
{
    m_listProc = new QProcess(this);
    m_transProc = new QProcess(this);
    connect(m_listProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FileTransfer::onListFinished);
    connect(m_transProc, &QProcess::readyReadStandardError,
            this, &FileTransfer::onTransferReadyRead);
    connect(m_transProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FileTransfer::onTransferFinished);
}

FileTransfer::~FileTransfer()
{
    cleanupAskpass();
}

static QString tempDir()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (d.isEmpty())
        d = QDir::currentPath();
    return d;
}

void FileTransfer::setupAskpassEnv(QProcess *proc)
{
    cleanupAskpass();
    if (m_session.password.isEmpty())
        return;

    QString base = QString("xbop_ft_%1_%2")
                       .arg(QCoreApplication::applicationPid())
                       .arg(reinterpret_cast<quintptr>(this));
    m_askpassPass = tempDir() + "/" + base + ".pass";
    m_askpassScript = tempDir() + "/" + base + ".cmd";

    QFile passFile(m_askpassPass);
    if (!passFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    passFile.write(m_session.password.toUtf8());
    passFile.close();
    passFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

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

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("SSH_ASKPASS", QDir::toNativeSeparators(m_askpassScript));
    env.insert("SSH_ASKPASS_REQUIRE", "force");
    env.insert("DISPLAY", "dummy");
    proc->setProcessEnvironment(env);
}

void FileTransfer::cleanupAskpass()
{
    if (!m_askpassPass.isEmpty()) {
        QFile::remove(m_askpassPass);
        m_askpassPass.clear();
    }
    if (!m_askpassScript.isEmpty()) {
        QFile::remove(m_askpassScript);
        m_askpassScript.clear();
    }
}

void FileTransfer::setSession(const Session &s)
{
    m_session = s;
}

QStringList FileTransfer::baseArgs() const
{
    QStringList args;
    args << "-p" << QString::number(m_session.port);
    args << "-o" << "StrictHostKeyChecking=no";
    args << "-o" << "UserKnownHostsFile=/dev/null";
    args << "-o" << "LogLevel=ERROR";
    args << "-o" << "NumberOfPasswordPrompts=3";
    if (!m_session.keyFile.isEmpty())
        args << "-i" << m_session.keyFile;
    return args;
}

void FileTransfer::listRemote(const QString &path)
{
    if (m_listProc->state() != QProcess::NotRunning)
        m_listProc->terminate();
    QString p = path.isEmpty() ? "." : path;
    QStringList args = baseArgs();
    args << QString("%1@%2").arg(m_session.user, m_session.host);
    args << QString("ls -la --time-style=long-iso %1").arg(p);
    setupAskpassEnv(m_listProc);
    m_listProc->start("ssh", args);
}

QList<FileEntry> FileTransfer::parseLs(const QString &text) const
{
    QList<FileEntry> out;
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    // 跳过总计行
    for (const QString &line : lines) {
        if (line.startsWith("total ") || line.startsWith("总计"))
            continue;
        QRegularExpression re("\\s+");
        QStringList parts = line.split(re, Qt::SkipEmptyParts);
        if (parts.size() < 6)
            continue;
        QString perm = parts[0];
        if (perm.size() < 1)
            continue;
        FileEntry e;
        e.isDir = perm.startsWith('d');
        e.isLink = perm.startsWith('l');
        e.size = parts[4];
        // 日期占两个字段：YYYY-MM-DD 与 HH:MM
        e.modTime = parts[5] + " " + parts[6];
        // 名称可能在符号链接后带 " -> target"
        QString nameField = parts.mid(7).join(" ");
        if (e.isLink && nameField.contains(" -> "))
            nameField = nameField.left(nameField.indexOf(" -> "));
        e.name = nameField;
        if (e.name.isEmpty())
            continue;
        out.append(e);
    }
    return out;
}

void FileTransfer::onListFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    cleanupAskpass();
    if (exitCode != 0) {
        emit done(false, "列目录失败：" + m_listProc->readAllStandardError());
        return;
    }
    // 需要知道当前路径：重新 ls 时已在命令里，这里用 "." 占位（UI 维护真实路径）
    emit listing(parseLs(m_listProc->readAllStandardOutput()), ".");
}

void FileTransfer::download(const QString &remotePath, const QString &localPath)
{
    QStringList args = baseArgs();
    args << QString("%1@%2:%3").arg(m_session.user, m_session.host, remotePath);
    args << localPath;
    m_transLocal = localPath;
    m_transRemote = remotePath;
    setupAskpassEnv(m_transProc);
    m_transProc->start("scp", args);
}

void FileTransfer::upload(const QString &localPath, const QString &remotePath)
{
    QStringList args = baseArgs();
    args << localPath;
    args << QString("%1@%2:%3").arg(m_session.user, m_session.host, remotePath);
    m_transLocal = localPath;
    m_transRemote = remotePath;
    setupAskpassEnv(m_transProc);
    m_transProc->start("scp", args);
}

void FileTransfer::downloadDir(const QString &remotePath, const QString &localDir)
{
    QStringList args = baseArgs();
    args << "-r";
    args << QString("%1@%2:%3").arg(m_session.user, m_session.host, remotePath);
    args << localDir;
    m_transLocal = localDir;
    m_transRemote = remotePath;
    setupAskpassEnv(m_transProc);
    m_transProc->start("scp", args);
}

void FileTransfer::uploadDir(const QString &localPath, const QString &remoteDir)
{
    QStringList args = baseArgs();
    args << "-r";
    args << localPath;
    args << QString("%1@%2:%3").arg(m_session.user, m_session.host, remoteDir);
    m_transLocal = localPath;
    m_transRemote = remoteDir;
    setupAskpassEnv(m_transProc);
    m_transProc->start("scp", args);
}

void FileTransfer::onTransferReadyRead()
{
    // scp 把进度输出到 stderr： "file  12%  1MB   1.0MB/s   00:05 ETA"
    QString line = QString::fromLocal8Bit(m_transProc->readAllStandardError());
    QRegularExpression re("(\\d+)%");
    QRegularExpressionMatch m = re.match(line);
    if (m.hasMatch())
        emit progress(m.captured(1).toInt(), line.trimmed());
}

void FileTransfer::onTransferFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    cleanupAskpass();
    if (exitCode == 0)
        emit done(true, QString("传输完成：%1").arg(m_transRemote));
    else
        emit done(false, "传输失败：" + m_transProc->readAllStandardError());
}

namespace RemoteDesktop {
    void launch(const Session &s)
    {
        QString type = s.desktopType;
        if (type == "auto")
            type = (s.os == "Windows") ? "RDP" : "VNC";
        if (type == "RDP") {
            // Windows 会话默认端口是 22（SSH），没改过时应走 RDP 默认 3389
            int rdpPort = (s.port == 22) ? 3389 : s.port;

            QStringList args;
            QString target = (rdpPort == 3389) ? s.host
                                               : QString("%1:%2").arg(s.host).arg(rdpPort);
            args << QString("/v:%1").arg(target);
            // mstsc 命令行不支持 /u: 传用户名，用户名在连接窗口输入或写入 .rdp 文件

            bool ok = false;
            const QStringList candidates = {
                "mstsc.exe",
                "C:/Windows/System32/mstsc.exe",
                "C:/Windows/SysWOW64/mstsc.exe"
            };
            for (const QString &prog : candidates) {
                ok = QProcess::startDetached(prog, args);
                if (ok)
                    break;
            }
            if (!ok) {
                XbMessageBox::warning(nullptr, "远程桌面启动失败",
                    QString("无法启动 mstsc.exe 连接 %1\n"
                            "请确认 Windows 远程桌面客户端可用。").arg(target));
            }
        } else {
            QString cmd = s.desktopCommand.isEmpty() ? "vncviewer" : s.desktopCommand;
            QString target = s.host;
            if (s.port != 5900)
                target = QString("%1:%2").arg(s.host).arg(s.port - 5900);
            if (!QProcess::startDetached(cmd, QStringList() << target)) {
                XbMessageBox::warning(nullptr, "VNC 启动失败",
                    QString("无法启动 %1，请检查命令是否存在。").arg(cmd));
            }
        }
    }
}
