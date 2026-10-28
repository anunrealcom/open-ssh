#pragma once

#include <QMainWindow>
#include <QMap>
#include <QList>

#include "session.h"
#include "conn.h"
#include "tunnel.h"

class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QLabel;
class QComboBox;
class QPushButton;
class QWidget;
class ConnSession;
class TerminalWidget;
class FileWidget;
class Tunnel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private slots:
    void onNewSession();
    void onQuickConnect();
    void onTreeConnect();
    void onTreeEdit();
    void onTreeDelete();
    void onTreeDesktop();
    void onRemoteDesktopCurrent();
    void onOpenSerial();
    void onOpenAI();
    void onNewTunnel();
    void onManageTunnels();
    void onTabClose(int index);
    void onTabChanged(int index);
    void onEncodingChanged(const QString &enc);

    void onSessionConnected();
    void onSessionDisconnected();
    void onSessionError(const QString &msg);

    // 无边框窗口控制
    void onMinimize();
    void onToggleMaximize();
    void onClose();

private:
    void setupTitleBar();
    void applyStyle(bool maximized = false);
    QRect btnRect(QWidget *w) const;   // 控件在窗口坐标系下的矩形

    void refreshTree();
    void openSession(const Session &s);
    Session currentSession() const;
    TerminalWidget *currentTerm() const;

    QTreeWidget *m_tree;
    QTabWidget *m_tabs;
    FileWidget *m_fileWidget;
    QWidget *m_rightPanel;             // 替代原 QDockWidget 文件面板
    QWidget *m_leftPanel;              // 替代原 QDockWidget 会话树
    QLabel *m_statusLabel;
    QComboBox *m_encodingCombo;

    QWidget *m_container = nullptr;    // 圆角+阴影容器
    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_minBtn = nullptr;
    QPushButton *m_maxBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;

    int m_counter = 0;
    QMap<int, Session> m_tabSessions;     // tabIndex -> session
    QMap<int, ConnSession *> m_tabConn;   // tabIndex -> 连接会话
    QList<Tunnel *> m_tunnels;            // 活动隧道
};
