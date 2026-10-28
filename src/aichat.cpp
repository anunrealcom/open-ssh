#include "aichat.h"
#include "ai.h"
#include "aidialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

AIChatWidget::AIChatWidget(QWidget *parent)
    : QWidget(parent)
{
    m_mgr = new QNetworkAccessManager(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部工具条
    auto *bar = new QWidget();
    bar->setObjectName("aiBar");
    bar->setFixedHeight(40);
    auto *bl = new QHBoxLayout(bar);
    bl->setContentsMargins(10, 0, 10, 0);
    m_modelLabel = new QLabel();
    m_modelLabel->setStyleSheet("QLabel{color:#0b76c4;font-weight:600;font-size:13px;}");
    auto *cfgBtn = new QPushButton("配置");
    auto *clearBtn = new QPushButton("清空");
    cfgBtn->setObjectName("primaryBtn");
    bl->addWidget(m_modelLabel);
    bl->addStretch(1);
    bl->addWidget(clearBtn);
    bl->addWidget(cfgBtn);
    connect(cfgBtn, &QPushButton::clicked, this, &AIChatWidget::onConfig);
    connect(clearBtn, &QPushButton::clicked, this, &AIChatWidget::onClear);

    // 消息区
    m_scroll = new QScrollArea();
    m_scroll->setWidgetResizable(true);
    m_scroll->setStyleSheet("QScrollArea{border:none;background:#fafbfc;}");
    m_msgContainer = new QWidget();
    m_msgLayout = new QVBoxLayout(m_msgContainer);
    m_msgLayout->setContentsMargins(12, 12, 12, 12);
    m_msgLayout->setSpacing(6);
    m_msgLayout->addStretch(1);
    m_scroll->setWidget(m_msgContainer);

    // 输入区
    auto *inputBox = new QWidget();
    inputBox->setObjectName("aiInputBox");
    auto *il = new QHBoxLayout(inputBox);
    il->setContentsMargins(10, 8, 10, 8);
    m_input = new QPlainTextEdit();
    m_input->setPlaceholderText("输入消息，Enter 发送，Shift+Enter 换行 …");
    m_input->setMaximumHeight(90);
    m_input->setStyleSheet(
        "QPlainTextEdit{background:#ffffff;border:1px solid #cfd8e3;"
        "border-radius:8px;padding:8px;font-size:13px;color:#2b3a4b;}");
    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setObjectName("primaryBtn");
    m_sendBtn->setFixedSize(72, 36);
    connect(m_sendBtn, &QPushButton::clicked, this, &AIChatWidget::onSend);
    il->addWidget(m_input, 1);
    il->addWidget(m_sendBtn);

    root->addWidget(bar);
    root->addWidget(m_scroll, 1);
    root->addWidget(inputBox);
    root->addWidget(m_statusLabel = new QLabel(""));
    m_statusLabel->setStyleSheet("QLabel{color:#8a97a6;font-size:11px;padding:0 10px 4px;}");

    // 当前模型显示
    AISettings cur = AIConfigStore::instance().current();
    m_modelLabel->setText(cur.isValid() ? ("AI 模型：" + cur.model) : "AI 模型：未配置");
}

void AIChatWidget::onConfig()
{
    AIDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        AISettings cur = AIConfigStore::instance().current();
        m_modelLabel->setText(cur.isValid() ? ("AI 模型：" + cur.model) : "AI 模型：未配置");
    }
}

void AIChatWidget::onClear()
{
    m_history.clear();
    QLayoutItem *child;
    while ((child = m_msgLayout->takeAt(0)) != nullptr) {
        if (child->layout()) {
            QLayout *sub = child->layout();
            while (sub->count()) {
                QLayoutItem *c2 = sub->takeAt(0);
                delete c2->widget();
            }
            delete sub;
        }
        delete child->widget();
        delete child;
    }
    m_msgLayout->addStretch(1);
}

void AIChatWidget::onSend()
{
    if (m_streaming)
        return;
    QString text = m_input->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    AISettings cfg = AIConfigStore::instance().current();
    if (!cfg.isValid()) {
        onConfig();
        return;
    }

    addBubble("user", text);
    m_history.append(qMakePair(QString("user"), text));
    m_input->clear();

    // 组装 messages
    QJsonArray messages;
    messages.append(QJsonObject{{"role", "system"},
                                {"content", "你是小白运维助手，一个专业的服务器运维与开发助手，"
                                            "回答简洁准确，必要时给出命令示例。"}});
    for (const auto &p : m_history)
        messages.append(QJsonObject{{"role", p.first}, {"content", p.second}});

    QJsonObject body;
    body["model"] = cfg.model;
    body["messages"] = messages;
    body["temperature"] = cfg.temperature;
    body["stream"] = true;

    QNetworkRequest req(QUrl(buildUrl(cfg.baseUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!cfg.apiKey.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + cfg.apiKey).toUtf8());

    m_reply = m_mgr->post(req, QJsonDocument(body).toJson());
    connect(m_reply, &QNetworkReply::readyRead, this, &AIChatWidget::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &AIChatWidget::onFinished);
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &AIChatWidget::onError);

    m_streaming = true;
    m_streamText.clear();
    m_streamLabel = addBubble("assistant", "…");
    m_sendBtn->setEnabled(false);
    m_statusLabel->setText("正在请求 " + cfg.model + " …");
}

void AIChatWidget::onReadyRead()
{
    if (!m_reply)
        return;
    m_streamBuf += QString::fromUtf8(m_reply->readAll());

    // 按行解析 SSE（保留最后不完整的一行）
    QStringList lines = m_streamBuf.split("\n");
    m_streamBuf = lines.last();
    for (int i = 0; i < lines.size() - 1; ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty())
            continue;
        if (!line.startsWith("data:"))
            continue;
        QString payload = line.mid(5).trimmed();
        if (payload == "[DONE]") {
            finishStream();
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
        if (doc.isObject()) {
            QJsonArray choices = doc.object().value("choices").toArray();
            if (!choices.isEmpty()) {
                QJsonObject ch = choices.first().toObject();
                QString delta = ch.value("delta").toObject().value("content").toString();
                if (!delta.isEmpty())
                    appendStream(delta);
            }
        }
    }
}

void AIChatWidget::onFinished()
{
    if (!m_reply)
        return;
    // 处理缓冲中剩余内容
    QStringList lines = m_streamBuf.split("\n");
    for (const QString &raw : lines) {
        QString line = raw.trimmed();
        if (line.startsWith("data:")) {
            QString payload = line.mid(5).trimmed();
            if (payload != "[DONE]") {
                QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
                if (doc.isObject()) {
                    QJsonArray choices = doc.object().value("choices").toArray();
                    if (!choices.isEmpty()) {
                        QString delta = choices.first().toObject()
                                            .value("delta").toObject()
                                            .value("content").toString();
                        if (!delta.isEmpty())
                            appendStream(delta);
                    }
                }
            }
        }
    }
    m_streamBuf.clear();

    if (m_reply->error() != QNetworkReply::NoError) {
        QString errBody = QString::fromUtf8(m_reply->readAll());
        if (!errBody.isEmpty())
            appendStream("\n\n[错误] " + errBody.left(600));
        m_statusLabel->setText("请求失败：" + m_reply->errorString());
    } else {
        m_statusLabel->setText("已就绪");
    }
    finishStream();
}

void AIChatWidget::onError(QNetworkReply::NetworkError)
{
    if (m_reply)
        m_statusLabel->setText("网络错误：" + m_reply->errorString());
}

void AIChatWidget::finishStream()
{
    if (!m_streaming)
        return;
    m_streaming = false;
    m_sendBtn->setEnabled(true);
    if (m_streamLabel) {
        QString finalText = m_streamText;
        if (finalText == "…" || finalText.isEmpty())
            finalText = "（无内容返回）";
        m_history.append(qMakePair(QString("assistant"), finalText));
        m_streamLabel = nullptr;
    }
    m_reply->deleteLater();
    m_reply = nullptr;
}

QLabel *AIChatWidget::addBubble(const QString &role, const QString &text)
{
    auto *bubble = new QLabel(text);
    bubble->setWordWrap(true);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(560);
    bubble->setStyleSheet(role == "user"
        ? "QLabel{background:#11a7ea;color:#ffffff;padding:8px 12px;"
          "border-radius:10px;font-size:13px;line-height:1.5;}"
        : "QLabel{background:#f1f4f8;color:#2b3a4b;padding:8px 12px;"
          "border-radius:10px;font-size:13px;line-height:1.5;}");

    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 2, 0, 2);
    if (role == "user") {
        row->addStretch(1);
        row->addWidget(bubble);
    } else {
        row->addWidget(bubble);
        row->addStretch(1);
    }
    // 在末尾 stretch 之前插入
    m_msgLayout->insertLayout(m_msgLayout->count() - 1, row);
    scrollToBottom();
    return bubble;
}

void AIChatWidget::appendStream(const QString &text)
{
    m_streamText += text;
    if (m_streamLabel)
        m_streamLabel->setText(m_streamText);
    scrollToBottom();
}

QString AIChatWidget::buildUrl(const QString &baseUrl) const
{
    QString u = baseUrl.trimmed();
    if (u.endsWith("/"))
        u.chop(1);
    if (u.endsWith("/chat/completions"))
        return u;
    return u + "/chat/completions";
}

void AIChatWidget::scrollToBottom()
{
    m_scroll->verticalScrollBar()->setValue(m_scroll->verticalScrollBar()->maximum());
}

void AIChatWidget::keyPressEvent(QKeyEvent *e)
{
    // Enter 发送，Shift+Enter 换行
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (!(e->modifiers() & Qt::ShiftModifier)) {
            onSend();
            e->accept();
            return;
        }
    }
    QWidget::keyPressEvent(e);
}
