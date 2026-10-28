#pragma once

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPair>
#include <QString>
#include <QKeyEvent>
#include <QScrollBar>

class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QPlainTextEdit;
class QLabel;
class QPushButton;

/**
 * AI 聊天面板：消息气泡 + 输入框，通过 QNetworkAccessManager
 * 调用 OpenAI 兼容 /chat/completions 接口，支持流式（SSE）输出。
 * 配置来自 AIConfigStore（多套配置，可在面板内打开配置对话框）。
 */
class AIChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AIChatWidget(QWidget *parent = nullptr);

private slots:
    void onSend();
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError err);
    void onConfig();
    void onClear();

private:
    QLabel *addBubble(const QString &role, const QString &text);
    void appendStream(const QString &text);
    QString buildUrl(const QString &baseUrl) const;
    void scrollToBottom();
    void finishStream();
    void keyPressEvent(QKeyEvent *e) override;

    QNetworkAccessManager *m_mgr = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_msgContainer = nullptr;
    QVBoxLayout *m_msgLayout = nullptr;
    QPlainTextEdit *m_input = nullptr;
    QPushButton *m_sendBtn = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_statusLabel = nullptr;

    QNetworkReply *m_reply = nullptr;
    QLabel *m_streamLabel = nullptr;     // 当前 assistant 气泡
    QString m_streamText;                // 流式累积文本
    QString m_streamBuf;                 // SSE 行缓冲（跨包）
    bool m_streaming = false;

    QList<QPair<QString, QString>> m_history; // role, content
};
