#include "sessiondialog.h"

#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>

SessionDialog::SessionDialog(QWidget *parent, const Session &s, bool quick)
    : FramelessDialog(parent), m_orig(s), m_quick(quick)
{
    setWindowTitle(quick ? "快速连接" : (s.id.isEmpty() ? "新建会话" : "编辑会话"));
    resize(420, 360);

    m_name = new QLineEdit(s.name);
    m_host = new QLineEdit(s.host);
    m_port = new QSpinBox();
    m_port->setRange(1, 65535);
    m_port->setValue(s.port ? s.port : 22);
    m_user = new QLineEdit(s.user);
    m_password = new QLineEdit(s.password);
    m_password->setEchoMode(QLineEdit::Password);
    m_key = new QLineEdit(s.keyFile);
    m_os = new QComboBox();
    m_os->addItems({"Linux", "Windows"});
    m_os->setCurrentText(s.os.isEmpty() ? "Linux" : s.os);
    m_deskType = new QComboBox();
    m_deskType->addItems({"auto", "RDP", "VNC"});
    m_deskType->setCurrentText(s.desktopType.isEmpty() ? "auto" : s.desktopType);
    m_deskCmd = new QLineEdit(s.desktopCommand);
    m_deskCmd->setPlaceholderText("VNC 时填查看器命令，如 vncviewer");
    m_group = new QLineEdit(s.group);
    m_group->setPlaceholderText("如 生产环境 / 测试环境，留空归为默认分组");
    m_save = new QCheckBox("保存到会话列表");
    m_save->setChecked(!quick);

    auto *browse = new QPushButton("浏览...");
    connect(browse, &QPushButton::clicked, this, &SessionDialog::onBrowseKey);
    auto *keyLayout = new QHBoxLayout();
    keyLayout->addWidget(m_key);
    keyLayout->addWidget(browse);

    auto *form = new QFormLayout();
    form->addRow("名称", m_name);
    form->addRow("主机", m_host);
    form->addRow("端口", m_port);
    form->addRow("用户", m_user);
    form->addRow("密码", m_password);
    form->addRow("私钥文件", keyLayout);
    form->addRow("操作系统", m_os);
    form->addRow("远程桌面", m_deskType);
    form->addRow("桌面命令", m_deskCmd);
    form->addRow("分组", m_group);
    form->addRow(m_save);

    auto *ok = new QPushButton(quick ? "连接" : "确定");
    auto *cancel = new QPushButton("取消");
    connect(ok, &QPushButton::clicked, this, &SessionDialog::onAccept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(ok);
    btnLayout->addWidget(cancel);

    auto *main = contentLayout();
    main->addLayout(form);
    main->addLayout(btnLayout);

    // 切换操作系统时，自动把默认端口改成对应协议的常用端口
    connect(m_os, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index == 1 && m_port->value() == 22)      // Windows -> RDP 3389
            m_port->setValue(3389);
        else if (index == 0 && m_port->value() == 3389) // Linux -> SSH 22
            m_port->setValue(22);
    });
}

Session SessionDialog::getSession() const
{
    Session s = m_orig;
    s.name = m_name->text().trimmed();
    s.host = m_host->text().trimmed();
    s.port = m_port->value();
    s.user = m_user->text().trimmed();
    s.password = m_password->text();
    s.keyFile = m_key->text().trimmed();
    s.os = m_os->currentText();
    s.desktopType = m_deskType->currentText();
    s.desktopCommand = m_deskCmd->text().trimmed();
    s.group = m_group->text().trimmed();
    if (s.group.isEmpty())
        s.group = "默认分组";
    if (s.name.isEmpty())
        s.name = QString("%1@%2").arg(s.user, s.host);
    return s;
}

void SessionDialog::onBrowseKey()
{
    QString f = QFileDialog::getOpenFileName(this, "选择私钥文件", QString(), "All (*.*)");
    if (!f.isEmpty())
        m_key->setText(f);
}

void SessionDialog::onAccept()
{
    if (m_host->text().trimmed().isEmpty()) {
        m_host->setFocus();
        return;
    }
    accept();
}
