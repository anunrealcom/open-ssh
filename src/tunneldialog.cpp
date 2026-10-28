#include "tunneldialog.h"

#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>

#include "session.h"
#include "tunnel.h"

TunnelDialog::TunnelDialog(QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle("新建端口转发（隧道）");
    resize(420, 280);

    m_session = new QComboBox();
    auto sessions = SessionStore::instance().all();
    for (const Session &s : sessions)
        m_session->addItem(s.name.isEmpty() ? s.host : s.name, s.id);
    if (m_session->count() == 0)
        m_session->addItem("(无会话，请先新建)", QString());

    m_type = new QComboBox();
    m_type->addItem("本地转发 (-L)", static_cast<int>(Tunnel::Local));
    m_type->addItem("远程转发 (-R)", static_cast<int>(Tunnel::Remote));

    m_localAddr = new QLineEdit("127.0.0.1");
    m_localPort = new QSpinBox();
    m_localPort->setRange(1, 65535);
    m_localPort->setValue(8080);

    m_remoteHost = new QLineEdit("127.0.0.1");
    m_remotePort = new QSpinBox();
    m_remotePort->setRange(1, 65535);
    m_remotePort->setValue(80);

    auto *form = new QFormLayout();
    form->addRow("会话", m_session);
    form->addRow("类型", m_type);
    form->addRow("本地绑定地址", m_localAddr);
    form->addRow("本地端口", m_localPort);
    form->addRow("目标主机", m_remoteHost);
    form->addRow("目标端口", m_remotePort);

    auto *ok = new QPushButton("启动");
    auto *cancel = new QPushButton("取消");
    connect(ok, &QPushButton::clicked, this, &TunnelDialog::onAccept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    auto *btn = new QHBoxLayout();
    btn->addStretch();
    btn->addWidget(ok);
    btn->addWidget(cancel);

    auto *main = contentLayout();
    main->addLayout(form);
    main->addLayout(btn);
}

TunnelConfig TunnelDialog::getConfig() const
{
    TunnelConfig c;
    QString id = m_session->currentData().toString();
    c.session = SessionStore::instance().byId(id);
    c.type = static_cast<Tunnel::Type>(m_type->currentData().toInt());
    c.localAddr = m_localAddr->text().trimmed();
    c.localPort = m_localPort->value();
    c.remoteHost = m_remoteHost->text().trimmed();
    c.remotePort = m_remotePort->value();
    return c;
}

void TunnelDialog::onAccept()
{
    if (m_session->currentData().toString().isEmpty()) {
        QMessageBox::information(this, "提示", "请先在主界面新建一个 SSH 会话。");
        return;
    }
    accept();
}

TunnelManagerDialog::TunnelManagerDialog(QList<Tunnel *> *tunnels, QWidget *parent)
    : FramelessDialog(parent), m_tunnels(tunnels)
{
    setWindowTitle("活动隧道");
    resize(420, 240);

    m_list = new QListWidget();
    auto *stop = new QPushButton("停止选中");
    auto *close = new QPushButton("关闭");
    connect(stop, &QPushButton::clicked, this, &TunnelManagerDialog::onStop);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);

    auto *btn = new QHBoxLayout();
    btn->addStretch();
    btn->addWidget(stop);
    btn->addWidget(close);

    auto *main = contentLayout();
    main->addWidget(m_list, 1);
    main->addLayout(btn);

    refresh();
}

void TunnelManagerDialog::refresh()
{
    m_list->clear();
    for (int i = 0; i < m_tunnels->size(); ++i) {
        Tunnel *t = m_tunnels->at(i);
        auto *it = new QListWidgetItem(t->description());
        it->setData(Qt::UserRole, i);
        m_list->addItem(it);
    }
    if (m_list->count() == 0)
        m_list->addItem("(无活动隧道)");
}

void TunnelManagerDialog::onStop()
{
    auto *item = m_list->currentItem();
    if (!item)
        return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_tunnels->size())
        return;
    Tunnel *t = m_tunnels->at(idx);
    t->stop();
    t->deleteLater();
    m_tunnels->removeAt(idx);
    refresh();
}
