#include "session.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>

QJsonObject Session::toJson() const
{
    QJsonObject o;
    o["id"] = id;
    o["name"] = name;
    o["host"] = host;
    o["user"] = user;
    o["password"] = password;
    o["keyFile"] = keyFile;
    o["port"] = port;
    o["os"] = os;
    o["desktopType"] = desktopType;
    o["desktopCommand"] = desktopCommand;
    o["group"] = group;
    return o;
}

void Session::fromJson(const QJsonObject &o)
{
    id = o.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    name = o.value("name").toString();
    host = o.value("host").toString();
    user = o.value("user").toString();
    password = o.value("password").toString();
    keyFile = o.value("keyFile").toString();
    port = o.value("port").toInt(22);
    os = o.value("os").toString("Linux");
    desktopType = o.value("desktopType").toString("auto");
    desktopCommand = o.value("desktopCommand").toString();
    group = o.value("group").toString("默认分组");
}

SessionStore &SessionStore::instance()
{
    static SessionStore s;
    return s;
}

SessionStore::SessionStore()
{
    load();
}

QString SessionStore::filePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/xiaobaiOP_sessions.json";
}

void SessionStore::load()
{
    m_sessions.clear();
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isArray())
        return;
    for (const QJsonValue &v : doc.array()) {
        if (v.isObject()) {
            Session s;
            s.fromJson(v.toObject());
            m_sessions.append(s);
        }
    }
}

void SessionStore::save()
{
    QJsonArray arr;
    for (const Session &s : m_sessions)
        arr.append(s.toJson());
    QFile f(filePath());
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    f.close();
}

QList<Session> SessionStore::all() const
{
    return m_sessions;
}

Session SessionStore::byId(const QString &id) const
{
    for (const Session &s : m_sessions)
        if (s.id == id)
            return s;
    return Session{};
}

void SessionStore::add(const Session &s)
{
    Session copy = s;
    if (copy.id.isEmpty())
        copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_sessions.append(copy);
    save();
}

void SessionStore::update(const Session &s)
{
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == s.id) {
            m_sessions[i] = s;
            save();
            return;
        }
    }
    add(s);
}

void SessionStore::remove(const QString &id)
{
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id == id) {
            m_sessions.removeAt(i);
            save();
            return;
        }
    }
}
