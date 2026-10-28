#pragma once

#include "framelessdialog.h"

#include "serialsession.h"

/**
 * 串口连接配置对话框。
 */
class SerialDialog : public FramelessDialog
{
    Q_OBJECT
public:
    explicit SerialDialog(QWidget *parent = nullptr);

    SerialSession::Config getConfig() const;

private slots:
    void onAccept();

private:
    class QLineEdit *m_port;
    class QComboBox *m_baud;
    class QComboBox *m_data;
    class QComboBox *m_stop;
    class QComboBox *m_parity;
};