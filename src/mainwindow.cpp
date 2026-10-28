#include "mainwindow.h"

#include <QTreeWidget>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include "xbmessagebox.h"
#include "aichat.h"
#include <QInputDialog>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

#include "sessiondialog.h"
#include "sshsession.h"
#include "terminalwidget.h"
#include "filewidget.h"
#include "filetransfer.h"
#include "conn.h"
#include "tunneldialog.h"
#include "serialdialog.h"
#include "serialsession.h"
#include "framelessdialog.h"

// 无边框窗口需要 Windows 原生消息处理（边缘缩放）
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1100, 720);
    setWindowTitle("小白运维 xiaobaiOP —— 单机 SSH 运维工具");
    setWindowIcon(qApp->windowIcon());

    // 无边框 + 透明背景（自绘圆角与阴影）
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 圆角 + 阴影容器
    m_container = new QWidget(this);
    m_container->setObjectName("container");
    setCentralWidget(m_container);
    auto *vlay = new QVBoxLayout(m_container);
    vlay->setContentsMargins(14, 14, 14, 14);
    vlay->setSpacing(0);

    // 标题栏
    setupTitleBar();
    vlay->addWidget(m_titleBar);

    // 工具栏（内嵌，不占用系统标题栏位置）
    auto *tb = new QToolBar();
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->setIconSize(QSize(20, 20));

    const QColor toolIconColor("#0b76c4");
    const int toolIconSize = 20;
    tb->addAction(xbMakeIcon("newSession", toolIconColor, toolIconSize), "新建会话", this, &MainWindow::onNewSession);
    tb->addAction(xbMakeIcon("quick", toolIconColor, toolIconSize), "快速连接", this, &MainWindow::onQuickConnect);
    tb->addAction(xbMakeIcon("desktop", toolIconColor, toolIconSize), "远程桌面", this, &MainWindow::onRemoteDesktopCurrent);
    tb->addAction(xbMakeIcon("files", toolIconColor, toolIconSize), "文件传输", [this]() { m_rightPanel->setVisible(!m_rightPanel->isVisible()); });
    tb->addSeparator();
    m_encodingCombo = new QComboBox();
    m_encodingCombo->addItems({"UTF-8", "GBK", "GB2312", "Big5", "Latin1"});
    m_encodingCombo->setCurrentText("UTF-8");
    m_encodingCombo->setToolTip("终端输出/输入编码");
    connect(m_encodingCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onEncodingChanged);
    tb->addWidget(new QLabel("编码:"));
    tb->addWidget(m_encodingCombo);
    tb->addSeparator();
    tb->addAction(xbMakeIcon("tunnel", toolIconColor, toolIconSize), "端口转发", this, &MainWindow::onNewTunnel);
    tb->addAction(xbMakeIcon("tunnels", toolIconColor, toolIconSize), "隧道管理", this, &MainWindow::onManageTunnels);
    tb->addAction(xbMakeIcon("serial", toolIconColor, toolIconSize), "串口", this, &MainWindow::onOpenSerial);
    tb->addSeparator();
    tb->addAction(xbMakeIcon("ai", toolIconColor, toolIconSize), "AI 助手", this, &MainWindow::onOpenAI);
    vlay->addWidget(tb);

    // 主体：左侧会话树 / 中央终端 / 右侧文件
    auto *body = new QWidget();
    auto *hlay = new QHBoxLayout(body);
    hlay->setContentsMargins(0, 8, 0, 8);
    hlay->setSpacing(8);

    m_tree = new QTreeWidget();
    m_tree->setHeaderLabel("会话列表");
    m_tree->setIconSize(QSize(20, 20));
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::customContextMenuRequested,
            this, [this](const QPoint &p) {
                auto *item = m_tree->itemAt(p);
                if (!item || item->data(0, Qt::UserRole).toString().isEmpty())
                    return;
                QMenu menu;
                menu.addAction("连接", this, &MainWindow::onTreeConnect);
                menu.addAction("编辑", this, &MainWindow::onTreeEdit);
                menu.addAction("远程桌面", this, &MainWindow::onTreeDesktop);
                menu.addSeparator();
                menu.addAction("删除", this, &MainWindow::onTreeDelete);
                menu.exec(m_tree->viewport()->mapToGlobal(p));
            });
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem *item) {
                if (!item->data(0, Qt::UserRole).toString().isEmpty())
                    onTreeConnect();
            });
    m_leftPanel = new QWidget();
    auto *llay = new QVBoxLayout(m_leftPanel);
    llay->setContentsMargins(0, 0, 0, 0);
    llay->addWidget(m_tree);
    m_leftPanel->setFixedWidth(220);
    hlay->addWidget(m_leftPanel, 0);

    m_tabs = new QTabWidget();
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabClose);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    hlay->addWidget(m_tabs, 1);

    m_fileWidget = new FileWidget();
    m_rightPanel = new QWidget();
    auto *rlay = new QVBoxLayout(m_rightPanel);
    rlay->setContentsMargins(0, 0, 0, 0);
    rlay->addWidget(m_fileWidget);
    m_rightPanel->setFixedWidth(340);
    hlay->addWidget(m_rightPanel, 0);
    m_rightPanel->hide();

    vlay->addWidget(body, 1);

    // 状态栏（收进圆角容器内）
    m_statusLabel = new QLabel("就绪");
    m_statusLabel->setObjectName("statusBar");
    m_statusLabel->setFixedHeight(24);
    vlay->addWidget(m_statusLabel);

    applyStyle(false);
    refreshTree();
}

void MainWindow::setupTitleBar()
{
    m_titleBar = new QWidget();
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(42);
    auto *hl = new QHBoxLayout(m_titleBar);
    hl->setContentsMargins(14, 0, 10, 0);
    hl->setSpacing(8);

    auto *icon = new QLabel();
    icon->setFixedSize(22, 22);
    icon->setAlignment(Qt::AlignCenter);
    icon->setPixmap(xbMakeIcon("logo", QColor("#11a7ea"), 20).pixmap(20, 20));
    icon->setStyleSheet("QLabel{background:#ffffff;border-radius:11px;}");
    m_titleLabel = new QLabel("小白运维 xiaobaiOP");
    m_titleLabel->setObjectName("titleLabel");

    hl->addWidget(icon);
    hl->addWidget(m_titleLabel);
    hl->addStretch(1);

    m_minBtn = new QPushButton();
    m_maxBtn = new QPushButton();
    m_closeBtn = new QPushButton();
    m_minBtn->setIcon(xbMakeIcon("min", QColor("#0b3a55"), 18));
    m_maxBtn->setIcon(xbMakeIcon("max", QColor("#0b3a55"), 18));
    m_closeBtn->setIcon(xbMakeIcon("close", QColor("#0b3a55"), 18));
    for (auto *b : {m_minBtn, m_maxBtn, m_closeBtn}) {
        b->setFixedSize(40, 30);
        b->setIconSize(QSize(18, 18));
        b->setCursor(Qt::PointingHandCursor);
    }
    m_minBtn->setObjectName("titleBtn");
    m_maxBtn->setObjectName("titleBtn");
    m_closeBtn->setObjectName("titleCloseBtn");
    connect(m_minBtn, &QPushButton::clicked, this, &MainWindow::onMinimize);
    connect(m_maxBtn, &QPushButton::clicked, this, &MainWindow::onToggleMaximize);
    connect(m_closeBtn, &QPushButton::clicked, this, &MainWindow::onClose);

    hl->addWidget(m_minBtn);
    hl->addWidget(m_maxBtn);
    hl->addWidget(m_closeBtn);
}

void MainWindow::applyStyle(bool maximized)
{
    const int radius = maximized ? 0 : 12;
    const int margin = maximized ? 0 : 14;

    if (auto *vl = qobject_cast<QVBoxLayout *>(m_container->layout()))
        vl->setContentsMargins(margin, margin, margin, margin);

    m_container->setStyleSheet(QString(
        "QWidget#container{background:#ffffff;border-radius:%1px;}").arg(radius));

    // 标题栏按钮：默认带半透明白底，图标（白色）始终可见；hover 背景变亮，关闭按钮 hover 变红
    m_titleBar->setStyleSheet(QString(
        "QWidget#m_titleBar{"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2ec4f6,stop:1 #11a7ea);"
        "  border-top-left-radius:%1px;border-top-right-radius:%1px;}"
        "QLabel#m_titleLabel{color:#ffffff;font-size:13px;font-weight:600;}"
        "QPushButton#titleBtn{"
        "  background:rgba(255,255,255,0.45);border:1px solid rgba(255,255,255,0.55);"
        "  border-radius:6px;padding:0;}"
        "QPushButton#titleBtn:hover{background:rgba(255,255,255,0.75);}"
        "QPushButton#titleCloseBtn{"
        "  background:rgba(255,255,255,0.45);border:1px solid rgba(255,255,255,0.55);"
        "  border-radius:6px;padding:0;}"
        "QPushButton#titleCloseBtn:hover{background:#e81123;border-color:#e81123;}").arg(radius));

    m_statusLabel->setStyleSheet(QString(
        "QLabel#statusBar{background:#f5f8fb;color:#5b6b7c;font-size:12px;padding-left:10px;"
        "border-top:1px solid #e3e8ef;border-bottom-left-radius:%1px;"
        "border-bottom-right-radius:%1px;}").arg(radius));

    m_leftPanel->setStyleSheet("QWidget{background:#ffffff;border:1px solid #e3e8ef;"
                               "border-radius:8px;}");
    m_rightPanel->setStyleSheet("QWidget{background:#ffffff;border:1px solid #e3e8ef;"
                                "border-radius:8px;}");
    m_tree->setStyleSheet(
        "QTreeWidget{background:transparent;border:none;font-size:13px;outline:none;}"
        "QHeaderView::section{background:#f2f6fb;color:#5b6b7c;border:none;padding:4px;}"
        "QTreeWidget::item{padding:3px 2px;}"
        "QTreeWidget::item:hover{background:#eaf6ff;}"
        "QTreeWidget::item:selected{background:#d4ecff;color:#0b76c4;}");

    if (auto *tb = qobject_cast<QToolBar *>(m_container->findChild<QToolBar *>())) {
        tb->setStyleSheet(
            "QToolBar{background:#f5f8fb;border:none;border-bottom:1px solid #e3e8ef;"
            "spacing:4px;padding:2px 4px;}"
            "QToolButton{background:transparent;border:none;padding:4px 10px;border-radius:6px;"
            "  color:#2b3a4b;font-size:13px;icon-size:20px;}"
            "QToolButton:hover{background:#e3f0fb;color:#0b76c4;}"
            "QLabel{color:#5b6b7c;padding:0 2px;}"
            "QComboBox{height:24px;min-width:80px;border:1px solid #cfd8e3;border-radius:5px;"
            "  background:#ffffff;padding:0 4px;}");
    }

    // 阴影
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 60));
    m_container->setGraphicsEffect(shadow);
}

QRect MainWindow::btnRect(QWidget *w) const
{
    return QRect(w->mapTo(this, QPoint(0, 0)), w->size());
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCHITTEST) {
        const int B = 6; // 边缘缩放热区
        long x = static_cast<short>(LOWORD(msg->lParam));
        long y = static_cast<short>(HIWORD(msg->lParam));
        QPoint p = mapFromGlobal(QPoint(x, y));
        QRect r = rect();

        if (isMaximized()) {
            // 最大化时仅标题栏可拖动/双击，边缘不缩放
            QRect tr(m_titleBar->mapTo(this, QPoint(0, 0)), m_titleBar->size());
            if (tr.contains(p)) {
                *result = HTCAPTION;
                return true;
            }
            *result = HTCLIENT;
            return true;
        }

        bool left = p.x() < B, right = p.x() > r.width() - B;
        bool top = p.y() < B, bottom = p.y() > r.height() - B;
        if (top && left)     { *result = HTTOPLEFT;     return true; }
        if (top && right)    { *result = HTTOPRIGHT;    return true; }
        if (bottom && left)  { *result = HTBOTTOMLEFT;  return true; }
        if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
        if (left)            { *result = HTLEFT;        return true; }
        if (right)           { *result = HTRIGHT;       return true; }
        if (top)             { *result = HTTOP;         return true; }
        if (bottom)          { *result = HTBOTTOM;      return true; }

        // 标题栏区域（排除按钮）作为拖动区
        QRect tr(m_titleBar->mapTo(this, QPoint(0, 0)), m_titleBar->size());
        if (tr.contains(p) &&
            !btnRect(m_closeBtn).contains(p) &&
            !btnRect(m_maxBtn).contains(p) &&
            !btnRect(m_minBtn).contains(p)) {
            *result = HTCAPTION;
            return true;
        }
        *result = HTCLIENT;
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    QPoint p = e->pos();
    QRect tr(m_titleBar->mapTo(this, QPoint(0, 0)), m_titleBar->size());
    if (tr.contains(p) &&
        !btnRect(m_closeBtn).contains(p) &&
        !btnRect(m_maxBtn).contains(p) &&
        !btnRect(m_minBtn).contains(p)) {
        onToggleMaximize();
    }
    QMainWindow::mouseDoubleClickEvent(e);
}

void MainWindow::onMinimize() { showMinimized(); }
void MainWindow::onClose()    { close(); }
void MainWindow::onToggleMaximize()
{
    if (isMaximized()) { showNormal(); applyStyle(false); }
    else               { showMaximized(); applyStyle(true); }
}

void MainWindow::refreshTree()
{
    m_tree->clear();
    auto *root = new QTreeWidgetItem(m_tree, QStringList("我的会话"));
    root->setFlags(root->flags() & ~Qt::ItemIsSelectable);
    root->setIcon(0, xbMakeIcon("group", QColor("#0b76c4"), 20));

    // 按分组归类，分组节点可折叠
    QMap<QString, QTreeWidgetItem *> groups;
    for (const Session &s : SessionStore::instance().all()) {
        const QString &g = s.group.isEmpty() ? "默认分组" : s.group;
        QTreeWidgetItem *gItem = groups.value(g);
        if (!gItem) {
            gItem = new QTreeWidgetItem(root, QStringList(g));
            gItem->setFlags(gItem->flags() & ~Qt::ItemIsSelectable);
            gItem->setIcon(0, xbMakeIcon("group", QColor("#0b76c4"), 20));
            gItem->setExpanded(true);
            groups[g] = gItem;
        }
        auto *item = new QTreeWidgetItem(gItem, QStringList(s.name.isEmpty() ? s.host : s.name));
        if (s.os == "Windows")
            item->setIcon(0, xbMakeIcon("windows", QColor("#11a7ea"), 20));
        else
            item->setIcon(0, xbMakeIcon("linux", QColor("#1a9e4b"), 20));
        item->setData(0, Qt::UserRole, s.id);
        item->setToolTip(0, QString("%1@%2:%3").arg(s.user, s.host).arg(s.port));
    }
    root->setExpanded(true);
}

void MainWindow::openSession(const Session &s)
{
    // Windows 会话默认走远程桌面（RDP），不再尝试 SSH 22 端口
    if (s.os == "Windows") {
        RemoteDesktop::launch(s);
        m_statusLabel->setText(QString("已启动远程桌面：%1").arg(s.host));
        return;
    }

    auto *conn = new SshSession(this);
    auto *term = new TerminalWidget();
    term->setSession(conn);
    term->setEncoding(m_encodingCombo->currentText());

    connect(conn, &ConnSession::connected, this, &MainWindow::onSessionConnected);
    connect(conn, &ConnSession::disconnected, this, &MainWindow::onSessionDisconnected);
    connect(conn, &ConnSession::errorOccurred, this, &MainWindow::onSessionError);

    int idx = m_tabs->addTab(term, s.name.isEmpty() ? s.host : s.name);
    m_tabSessions[idx] = s;
    m_tabConn[idx] = conn;
    m_tabs->setCurrentIndex(idx);

    conn->connectTo(s);
    m_statusLabel->setText(QString("正在连接 %1@%2 ...").arg(s.user, s.host));
}

void MainWindow::onNewSession()
{
    SessionDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Session s = dlg.getSession();
    SessionStore::instance().add(s);
    refreshTree();
    openSession(s);
}

void MainWindow::onQuickConnect()
{
    SessionDialog dlg(this, Session{}, true);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Session s = dlg.getSession();
    s.id.clear();
    openSession(s);
}

void MainWindow::onTreeConnect()
{
    auto *item = m_tree->currentItem();
    if (!item)
        return;
    QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty())
        return;
    openSession(SessionStore::instance().byId(id));
}

void MainWindow::onTreeEdit()
{
    auto *item = m_tree->currentItem();
    if (!item)
        return;
    QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty())
        return;
    Session s = SessionStore::instance().byId(id);
    SessionDialog dlg(this, s);
    if (dlg.exec() == QDialog::Accepted) {
        Session updated = dlg.getSession();
        updated.id = id;
        SessionStore::instance().update(updated);
        refreshTree();
    }
}

void MainWindow::onTreeDelete()
{
    auto *item = m_tree->currentItem();
    if (!item)
        return;
    QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty())
        return;
    if (XbMessageBox::question(this, "删除", "确定删除该会话？") == QDialog::Accepted) {
        SessionStore::instance().remove(id);
        refreshTree();
    }
}

void MainWindow::onTreeDesktop()
{
    auto *item = m_tree->currentItem();
    if (!item)
        return;
    QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty())
        return;
    RemoteDesktop::launch(SessionStore::instance().byId(id));
}

void MainWindow::onRemoteDesktopCurrent()
{
    Session s = currentSession();
    // 当前没有 SSH 标签页时，尝试取左侧树中选中的会话（方便 Windows 会话直接开 RDP）
    if (s.host.isEmpty()) {
        auto *item = m_tree->currentItem();
        if (item) {
            QString id = item->data(0, Qt::UserRole).toString();
            if (!id.isEmpty())
                s = SessionStore::instance().byId(id);
        }
    }
    if (s.host.isEmpty()) {
        XbMessageBox::information(this, "提示", "请先连接一个会话，或在左侧选择会话。");
        return;
    }
    RemoteDesktop::launch(s);
}

Session MainWindow::currentSession() const
{
    int idx = m_tabs->currentIndex();
    return m_tabSessions.value(idx, Session{});
}

TerminalWidget *MainWindow::currentTerm() const
{
    int idx = m_tabs->currentIndex();
    QWidget *w = m_tabs->widget(idx);
    return qobject_cast<TerminalWidget *>(w);
}

void MainWindow::onTabClose(int index)
{
    if (ConnSession *conn = m_tabConn.value(index)) {
        conn->closeConn();
        conn->deleteLater();
    }
    m_tabConn.remove(index);
    m_tabSessions.remove(index);
    m_tabs->removeTab(index);
}

void MainWindow::onTabChanged(int index)
{
    Session s = m_tabSessions.value(index, Session{});
    // Windows 会话不创建 SSH 标签页；Linux 会话才关联文件面板
    if (!s.host.isEmpty() && s.os != "Windows")
        m_fileWidget->setSession(s);
    // 同步编码下拉到当前终端
    if (TerminalWidget *term = qobject_cast<TerminalWidget *>(m_tabs->widget(index)))
        m_encodingCombo->setCurrentText(term->encoding());
}

void MainWindow::onEncodingChanged(const QString &enc)
{
    if (TerminalWidget *term = currentTerm())
        term->setEncoding(enc);
}

void MainWindow::onNewTunnel()
{
    TunnelDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    TunnelConfig cfg = dlg.getConfig();
    if (cfg.session.host.isEmpty()) {
        XbMessageBox::information(this, "提示", "会话无效，请先在主界面新建 SSH 会话。");
        return;
    }
    auto *t = new Tunnel(this);
    connect(t, &Tunnel::started, this, [this, t]() {
        m_statusLabel->setText("隧道已启动：" + t->description());
    });
    connect(t, &Tunnel::stopped, this, [this]() {
        m_statusLabel->setText("隧道已停止");
    });
    connect(t, &Tunnel::errorOccurred, this, [this](const QString &msg) {
        XbMessageBox::critical(this, "隧道错误", msg);
    });
    t->start(cfg.session, cfg.type, cfg.localAddr, cfg.localPort,
             cfg.remoteHost, cfg.remotePort);
    m_tunnels.append(t);
    m_statusLabel->setText("正在启动隧道 ...");
}

void MainWindow::onManageTunnels()
{
    TunnelManagerDialog dlg(&m_tunnels, this);
    dlg.exec();
}

void MainWindow::onOpenSerial()
{
    SerialDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    SerialSession::Config cfg = dlg.getConfig();

    auto *conn = new SerialSession(this);
    auto *term = new TerminalWidget();
    term->setSession(conn);
    term->setEncoding(m_encodingCombo->currentText());

    connect(conn, &ConnSession::connected, this, &MainWindow::onSessionConnected);
    connect(conn, &ConnSession::disconnected, this, &MainWindow::onSessionDisconnected);
    connect(conn, &ConnSession::errorOccurred, this, &MainWindow::onSessionError);

    int idx = m_tabs->addTab(term, "串口 " + cfg.port);
    m_tabConn[idx] = conn; // 串口无 Session 数据，不写入 m_tabSessions
    m_tabs->setCurrentIndex(idx);

    conn->start(cfg);
    m_statusLabel->setText(QString("正在打开串口 %1 ...").arg(cfg.port));
}

void MainWindow::onOpenAI()
{
    auto *dlg = new FramelessDialog(this, "AI 助手");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(820, 600);
    auto *chat = new AIChatWidget(dlg);
    dlg->contentLayout()->addWidget(chat);
    dlg->show();
}

void MainWindow::onSessionConnected()
{
    m_statusLabel->setText("已连接");
}

void MainWindow::onSessionDisconnected()
{
    m_statusLabel->setText("连接已断开");
}

void MainWindow::onSessionError(const QString &msg)
{
    m_statusLabel->setText("错误：" + msg);
    XbMessageBox::critical(this, "连接错误", msg);
}
