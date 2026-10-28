#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QProcess>

#include "session.h"

struct FileEntry
{
    QString name;
    QString size;
    QString modTime;
    bool isDir = false;
    bool isLink = false;
};

/**
 * 基于系统 scp / ssh 的文件传输（SFTP over SSH）。
 * 列出目录走 ssh "ls -la"，传输走 scp，零额外依赖。
 */
class FileTransfer : public QObject
{
    Q_OBJECT
public:
    explicit FileTransfer(QObject *parent = nullptr);
    ~FileTransfer();

    void setSession(const Session &s);
    void listRemote(const QString &path);
    void download(const QString &remotePath, const QString &localPath);
    void upload(const QString &localPath, const QString &remotePath);
    void downloadDir(const QString &remotePath, const QString &localDir);
    void uploadDir(const QString &localPath, const QString &remoteDir);

signals:
    void listing(const QList<FileEntry> &entries, const QString &path);
    void progress(int percent, const QString &info);
    void done(bool ok, const QString &msg);

private slots:
    void onListFinished(int exitCode, QProcess::ExitStatus status);
    void onTransferReadyRead();
    void onTransferFinished(int exitCode, QProcess::ExitStatus status);

private:
    QStringList baseArgs() const;
    QList<FileEntry> parseLs(const QString &text) const;
    void setupAskpassEnv(QProcess *proc);
    void cleanupAskpass();

    Session m_session;
    QProcess *m_listProc = nullptr;
    QProcess *m_transProc = nullptr;
    QString m_transLocal;
    QString m_transRemote;

    QString m_askpassScript;
    QString m_askpassPass;
};

/**
 * 远程桌面启动：Windows 走 mstsc（RDP），Linux 走配置的 VNC 命令。
 */
namespace RemoteDesktop {
    void launch(const Session &s);
}
