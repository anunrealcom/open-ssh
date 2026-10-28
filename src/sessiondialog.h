#pragma once

#include "framelessdialog.h"

#include "session.h"

/**
 * 新增 / 编辑会话的对话框，也可作为"快速连接"使用。
 */
class SessionDialog : public FramelessDialog
{
    Q_OBJECT
public:
    explicit SessionDialog(QWidget *parent = nullptr, const Session &s = Session{}, bool quick = false);

    Session getSession() const;

private slots:
    void onBrowseKey();
    void onAccept();

private:
    class QLineEdit *m_name, *m_host, *m_user, *m_password, *m_key, *m_deskCmd, *m_group;
    class QSpinBox *m_port;
    class QComboBox *m_os, *m_deskType;
    class QCheckBox *m_save;
    Session m_orig;
    bool m_quick;
};
