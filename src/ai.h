#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>

/**
 * 一套 AI 模型配置（OpenAI 兼容协议）：
 * 只需填写 API Base、API Key、模型名即可对接绝大多数厂商
 * （OpenAI / DeepSeek / 通义千问 / 混元 / Ollama / 本地 vLLM 等）。
 */
struct AISettings
{
    QString name;          // 配置名（唯一，用于识别多套配置）
    QString provider;      // 厂商标签（仅展示）
    QString baseUrl;       // 如 https://api.openai.com/v1
    QString apiKey;        // API Key（明文存于本机配置目录）
    QString model;         // 模型名，如 gpt-4o / deepseek-chat
    double temperature = 0.7;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &o);
    bool isValid() const { return !baseUrl.isEmpty() && !model.isEmpty(); }
};

/**
 * AI 配置持久化：多套配置 + 当前选择，存于本机 AppConfig 目录的
 * xiaobaiOP_ai.json。纯本地、单机，不依赖任何第三方服务。
 */
class AIConfigStore
{
public:
    static AIConfigStore &instance();

    QList<AISettings> all() const;
    AISettings current() const;
    QString currentName() const { return m_current; }
    void setCurrent(const QString &name);

    void save(const AISettings &s);   // 按 name upsert
    void remove(const QString &name);

    /** 常见厂商预设模板（baseUrl + 默认模型），用户可在此基础上改 */
    static QList<AISettings> presets();

private:
    AIConfigStore();
    void load();
    void saveAll();

    QList<AISettings> m_profiles;
    QString m_current;
};
