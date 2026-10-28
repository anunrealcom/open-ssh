#include "serialdialog.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

SerialDialog::SerialDialog(QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle("串口连接");
    resize(320, 240);

    m_port = new QLineEdit("COM3");
    m_baud = new QComboBox();
    m_baud->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    m_baud->setCurrentText("115200");
    m_data = new QComboBox();
    m_data->addItems({"5", "6", "7", "8"});
    m_data->setCurrentText("8");
    m_stop = new QComboBox();
    m_stop->addItems({"1", "1.5", "2"});
    m_stop->setCurrentText("1");
    m_parity = new QComboBox();
    m_parity->addItems({"无 (N)", "奇 (O)", "偶 (E)"});
    m_parity->setCurrentIndex(0);

    auto *form = new QFormLayout();
    form->addRow("端口", m_port);
    form->addRow("波特率", m_baud);
    form->addRow("数据位", m_data);
    form->addRow("停止位", m_stop);
    form->addRow("校验", m_parity);

    auto *ok = new QPushButton("连接");
    auto *cancel = new QPushButton("取消");
    connect(ok, &QPushButton::clicked, this, &SerialDialog::onAccept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    auto *btn = new QHBoxLayout();
    btn->addStretch();
    btn->addWidget(ok);
    btn->addWidget(cancel);

    auto *main = contentLayout();
    main->addLayout(form);
    main->addLayout(btn);
}

SerialSession::Config SerialDialog::getConfig() const
{
    SerialSession::Config c;
    c.port = m_port->text().trimmed();
    c.baudRate = m_baud->currentText().toInt();
    c.dataBits = m_data->currentText().toInt();
    double sb = m_stop->currentText().toDouble();
    c.stopBits = (sb == 2) ? 2 : (sb == 1.5 ? 1.5 : 1);
    int pi = m_parity->currentIndex();
    c.parity = (pi == 1) ? 'O' : (pi == 2 ? 'E' : 'N');
    return c;
}

void SerialDialog::onAccept()
{
    if (m_port->text().trimmed().isEmpty()) {
        m_port->setFocus();
        return;
    }
    accept();
}
