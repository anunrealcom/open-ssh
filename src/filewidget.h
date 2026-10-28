#pragma once

#include <QWidget>
#include <QList>
#include <QListWidget>

#include "session.h"
#include "filetransfer.h"

class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;

/**
 * 文件传输面板：左侧本地目录，右侧远程目录。
 * 远程列目录走 ssh ls，下载/上传走 scp。
 */
class FileWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FileWidget(QWidget *parent = nullptr);

public slots:
    void setSession(const Session &s);

private slots:
    void onRemoteCd(QListWidgetItem *item);
    void onRemoteUp();
    void onRemoteRefresh();
    void onDownload();
    void onUpload();
    void onRemoteListing(const QList<FileEntry> &entries, const QString &path);
    void onTransferDone(bool ok, const QString &msg);
    void onProgress(int percent, const QString &info);

private:
    void refreshRemote();
    void browseLocal();
    void refreshLocal();

    Session m_session;
    FileTransfer *m_ft = nullptr;
    QString m_remotePath = ".";
    QString m_localPath;

    QLineEdit *m_remotePathEdit;
    QLineEdit *m_localPathEdit;
    QListWidget *m_remoteList;
    QListWidget *m_localList;
    QProgressBar *m_progress;
    QLabel *m_status;
};
