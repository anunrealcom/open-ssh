#include "ai.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

QJsonObject AISettings::toJson() const
{
    QJsonObject o;
    o["name"] = name;
    o["provider"] = provider;
    o["baseUrl"] = baseUrl;
    o["apiKey"] = apiKey;
    o["model"] = model;
    o["temperature"] = temperature;
    return o;
}

void AISettings::fromJson(const QJsonObject &o)
{
    name = o.value("name").toString();
    provider = o.value("provider").toString();
    baseUrl = o.value("baseUrl").toString();
    apiKey = o.value("apiKey").toString();
    model = o.value("model").toString();
    temperature = o.value("temperature").toDouble(0.7);
}

AIConfigStore &AIConfigStore::instance()
{
    static AIConfigStore s;
    return s;
}

AIConfigStore::AIConfigStore() { load(); }

QString aiFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/xiaobaiOP_ai.json";
}

void AIConfigStore::load()
{
    m_profiles.clear();
    m_current.clear();
    QFile f(aiFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return;
    QJsonObject root = doc.object();
    m_current = root.value("current").toString();
    for (const QJsonValue &v : root.value("profiles").toArray()) {
        if (v.isObject()) {
            AISettings s;
            s.fromJson(v.toObject());
            if (!s.name.isEmpty())
                m_profiles.append(s);
        }
    }
    if (m_current.isEmpty() && !m_profiles.isEmpty())
        m_current = m_profiles.first().name;
}

void AIConfigStore::saveAll()
{
    QJsonObject root;
    root["current"] = m_current;
    QJsonArray arr;
    for (const AISettings &s : m_profiles)
        arr.append(s.toJson());
    root["profiles"] = arr;
    QFile f(aiFilePath());
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
}

QList<AISettings> AIConfigStore::all() const { return m_profiles; }

AISettings AIConfigStore::current() const
{
    for (const AISettings &s : m_profiles)
        if (s.name == m_current)
            return s;
    return AISettings{};
}

void AIConfigStore::setCurrent(const QString &name)
{
    m_current = name;
    saveAll();
}

void AIConfigStore::save(const AISettings &s)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].name == s.name) {
            m_profiles[i] = s;
            saveAll();
            return;
        }
    }
    m_profiles.append(s);
    if (m_current.isEmpty())
        m_current = s.name;
    saveAll();
}

void AIConfigStore::remove(const QString &name)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].name == name) {
            m_profiles.removeAt(i);
            break;
        }
    }
    if (m_current == name)
        m_current = m_profiles.isEmpty() ? QString() : m_profiles.first().name;
    saveAll();
}

QList<AISettings> AIConfigStore::presets()
{
    QList<AISettings> list;
    list.append({ "OpenAI", "OpenAI", "https://api.openai.com/v1", "", "gpt-4o", 0.7 });
    list.append({ "DeepSeek", "DeepSeek", "https://api.deepseek.com/v1", "", "deepseek-chat", 0.7 });
    list.append({ "通义千问", "阿里云百炼", "https://dashscope.aliyuncs.com/compatible-mode/v1", "", "qwen-plus", 0.7 });
    list.append({ "腾讯混元", "腾讯云", "https://api.hunyuan.cloud.tencent.com/v1", "", "hunyuan-lite", 0.7 });
    list.append({ "Ollama(本地)", "Ollama", "http://localhost:11434/v1", "ollama", "llama3.1", 0.7 });
    list.append({ "自定义", "自定义", "", "", "", 0.7 });
    return list;
}
