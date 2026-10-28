#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>

/**
 * 一个远程连接会话的配置。
 * 纯单机工具：会话信息仅保存在本机 JSON 文件，不上传任何服务端。
 */
struct Session
{
    QString id;
    QString name;          // 显示名
    QString host;          // 主机地址 / IP
    QString user;          // 登录用户
    QString password;      // 密码（明文存本机，单机工具可接受）
    QString keyFile;       // 私钥路径（为空则用密码）
    int port = 22;

    QString os = "Linux";  // Linux / Windows —— 用于决定默认远程桌面协议
    QString desktopType = "auto"; // auto / RDP / VNC
    QString desktopCommand;       // VNC 自定义命令（如 "vncviewer"）
    QString group = "默认分组";   // 会话分组（左侧树按分组折叠）

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &o);
};

/**
 * 会话存储：本机 JSON 文件持久化。
 */
class SessionStore
{
public:
    static SessionStore &instance();

    QList<Session> all() const;
    Session byId(const QString &id) const;
    void add(const Session &s);
    void update(const Session &s);
    void remove(const QString &id);
    void save();

private:
    SessionStore();
    QString filePath() const;
    void load();

    QList<Session> m_sessions;
};
