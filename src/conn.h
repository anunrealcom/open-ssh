#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

/**
 * 连接会话的抽象基类：任何能向 TerminalWidget 提供字节流的连接都继承它
 * （SSH、串口、telnet...）。TerminalWidget 只依赖本接口，与具体协议解耦。
 */
class ConnSession : public QObject
{
    Q_OBJECT
public:
    explicit ConnSession(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    // 向对端写入数据（键盘输入、命令等）
    virtual void write(const QByteArray &data) = 0;
    // 断开连接
    virtual void closeConn() = 0;

signals:
    void output(const QByteArray &data);
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);
};
