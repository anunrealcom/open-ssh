#include "filewidget.h"

#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>
#include "xbmessagebox.h"
#include <QFileInfo>
#include <QStandardPaths>

FileWidget::FileWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ft = new FileTransfer(this);
    connect(m_ft, &FileTransfer::listing, this, &FileWidget::onRemoteListing);
    connect(m_ft, &FileTransfer::done, this, &FileWidget::onTransferDone);
    connect(m_ft, &FileTransfer::progress, this, &FileWidget::onProgress);

    m_remotePathEdit = new QLineEdit(".");
    m_localPathEdit = new QLineEdit(
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    m_localPath = m_localPathEdit->text();
    m_remoteList = new QListWidget();
    m_localList = new QListWidget();
    m_progress = new QProgressBar();
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_status = new QLabel("未连接会话");

    auto *remoteUp = new QPushButton("↑上级");
    auto *remoteRefresh = new QPushButton("刷新");
    auto *localBrowse = new QPushButton("选择本地目录");
    auto *download = new QPushButton("下载 →");
    auto *upload = new QPushButton("← 上传");

    connect(m_remoteList, &QListWidget::itemDoubleClicked, this, &FileWidget::onRemoteCd);
    connect(remoteUp, &QPushButton::clicked, this, &FileWidget::onRemoteUp);
    connect(remoteRefresh, &QPushButton::clicked, this, &FileWidget::onRemoteRefresh);
    connect(localBrowse, &QPushButton::clicked, this, &FileWidget::browseLocal);
    connect(download, &QPushButton::clicked, this, &FileWidget::onDownload);
    connect(upload, &QPushButton::clicked, this, &FileWidget::onUpload);

    auto *rTop = new QHBoxLayout();
    rTop->addWidget(new QLabel("远程:"));
    rTop->addWidget(m_remotePathEdit, 1);
    rTop->addWidget(remoteUp);
    rTop->addWidget(remoteRefresh);

    auto *lTop = new QHBoxLayout();
    lTop->addWidget(new QLabel("本地:"));
    lTop->addWidget(m_localPathEdit, 1);
    lTop->addWidget(localBrowse);

    auto *mid = new QHBoxLayout();
    mid->addWidget(download);
    mid->addWidget(upload);

    auto *remoteCol = new QVBoxLayout();
    remoteCol->addLayout(rTop);
    remoteCol->addWidget(m_remoteList, 1);

    auto *localCol = new QVBoxLayout();
    localCol->addLayout(lTop);
    localCol->addWidget(m_localList, 1);

    auto *cols = new QHBoxLayout();
    cols->addLayout(remoteCol, 1);
    cols->addLayout(localCol, 1);

    auto *main = new QVBoxLayout(this);
    main->addLayout(cols);
    main->addLayout(mid);
    main->addWidget(m_progress);
    main->addWidget(m_status);
}

void FileWidget::setSession(const Session &s)
{
    m_session = s;
    m_ft->setSession(s);
    m_status->setText(QString("已关联会话：%1@%2").arg(s.user, s.host));

    // Windows 会话通过 RDP 连接，不走 SSH/scp，文件面板不发起远程列表
    if (s.os == "Windows") {
        m_remoteList->clear();
        m_remoteList->addItem("Windows 远程桌面不支持 SSH 文件传输");
        m_remoteList->addItem("请使用 RDP 的本地资源共享/剪贴板");
        m_remotePathEdit->setText("—");
        refreshLocal();
        return;
    }

    refreshRemote();
    refreshLocal();
}

void FileWidget::refreshRemote()
{
    if (m_session.host.isEmpty())
        return;
    m_remotePathEdit->setText(m_remotePath);
    m_ft->listRemote(m_remotePath);
}

void FileWidget::browseLocal()
{
    QString picked = QFileDialog::getExistingDirectory(this, "选择本地目录",
                                                       m_localPathEdit->text());
    if (picked.isEmpty())
        return;
    m_localPath = picked;
    refreshLocal();
}

void FileWidget::refreshLocal()
{
    m_localPathEdit->setText(m_localPath);
    QDir dir(m_localPath);
    m_localList->clear();
    m_localList->addItem("[..]");
    for (const QFileInfo &fi : dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                                 QDir::DirsFirst | QDir::Name)) {
        m_localList->addItem((fi.isDir() ? "[D] " : "") + fi.fileName());
    }
}

void FileWidget::onRemoteCd(QListWidgetItem *item)
{
    QString name = item->text();
    if (name == ".." || name == "[..]") {
        onRemoteUp();
        return;
    }
    if (name.startsWith("[D] ") || name.startsWith("d"))
        m_remotePath = (m_remotePath == "." ? "" : m_remotePath + "/") + name;
    refreshRemote();
}

void FileWidget::onRemoteUp()
{
    int idx = m_remotePath.lastIndexOf('/');
    m_remotePath = idx <= 0 ? "." : m_remotePath.left(idx);
    refreshRemote();
}

void FileWidget::onRemoteRefresh()
{
    refreshRemote();
}

void FileWidget::onDownload()
{
    auto *item = m_remoteList->currentItem();
    if (!item) {
        XbMessageBox::information(this, "提示", "请先在右侧选择一个远程文件。");
        return;
    }
    QString raw = item->text();
    QString name = raw;
    if (name.startsWith("[D] ")) name = name.mid(4);
    else if (name.startsWith("[L] ")) name = name.mid(4);
    QString remote = (m_remotePath == "." ? "" : m_remotePath + "/") + name;

    if (raw.startsWith("[D]")) {
        // 递归下载整个目录到当前本地目录
        m_ft->downloadDir(remote, m_localPath);
    } else {
        m_ft->download(remote, m_localPath + "/" + name);
    }
}

void FileWidget::onUpload()
{
    auto *item = m_localList->currentItem();
    if (!item)
        return;
    QString raw = item->text();
    if (raw == "[..]")
        return;
    QString name = raw;
    if (name.startsWith("[D] ")) name = name.mid(4);
    QString local = m_localPath + "/" + name;
    QString remote = (m_remotePath == "." ? "" : m_remotePath + "/") + name;

    if (raw.startsWith("[D]")) {
        m_ft->uploadDir(local, remote);
    } else {
        m_ft->upload(local, remote);
    }
}

void FileWidget::onRemoteListing(const QList<FileEntry> &entries, const QString &)
{
    m_remoteList->clear();
    m_remoteList->addItem("..");
    for (const FileEntry &e : entries) {
        QString label = (e.isDir ? "[D] " : (e.isLink ? "[L] " : "")) + e.name;
        auto *it = new QListWidgetItem(label);
        m_remoteList->addItem(it);
    }
}

void FileWidget::onTransferDone(bool ok, const QString &msg)
{
    m_progress->setValue(ok ? 100 : 0);
    m_status->setText(msg);
    if (ok) {
        refreshRemote();
        browseLocal();
    } else {
        XbMessageBox::warning(this, "传输", msg);
    }
}

void FileWidget::onProgress(int percent, const QString &info)
{
    m_progress->setValue(percent);
    m_status->setText(info);
}
