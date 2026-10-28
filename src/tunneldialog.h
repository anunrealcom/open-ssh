#pragma once

#include <QList>

#include "framelessdialog.h"
#include "session.h"
#include "tunnel.h"

struct TunnelConfig
{
    Session session;
    Tunnel::Type type = Tunnel::Local;
    QString localAddr = "127.0.0.1";
    int localPort = 8080;
    QString remoteHost = "127.0.0.1";
    int remotePort = 80;
};

/**
 * 新建端口转发（隧道）对话框。
 */
class TunnelDialog : public FramelessDialog
{
    Q_OBJECT
public:
    explicit TunnelDialog(QWidget *parent = nullptr);

    TunnelConfig getConfig() const;

private slots:
    void onAccept();

private:
    class QComboBox *m_session;
    class QComboBox *m_type;
    class QLineEdit *m_localAddr;
    class QSpinBox *m_localPort;
    class QLineEdit *m_remoteHost;
    class QSpinBox *m_remotePort;
};

/**
 * 活动隧道管理（停止）对话框。
 */
class TunnelManagerDialog : public FramelessDialog
{
    Q_OBJECT
public:
    explicit TunnelManagerDialog(QList<Tunnel *> *tunnels, QWidget *parent = nullptr);

private slots:
    void refresh();
    void onStop();

private:
    QList<Tunnel *> *m_tunnels;
    class QListWidget *m_list;
};
