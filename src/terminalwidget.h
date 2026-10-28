#pragma once

#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QInputMethodEvent>

#include "conn.h"

class ConnSession;

/**
 * 轻量终端视图：在 QPlainTextEdit 上叠加一个最小化 ANSI/VT100 解析器。
 * 支持：换行/回车/退格、SGR 颜色（加粗、前景/背景 16 色）、行内擦除。
 * 光标移动类转义（上下左右、定位）按"无副作用"方式消费，保证常见 shell 可用。
 * 这是一种务实实现，不追求 100% 兼容全屏程序（vim/top 基本可用）。
 */
class TerminalWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);

    void setSession(ConnSession *session);

    // 输出/输入编码（UTF-8 / GBK / GB2312 / Big5 / Latin1 ...）
    void setEncoding(const QString &name);
    QString encoding() const;

public slots:
    void appendOutput(const QByteArray &data);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void inputMethodEvent(QInputMethodEvent *e) override;

private:
    void insertText(const QString &text, const QTextCharFormat &fmt);
    void applySGR(const QStringList &codes);
    void handleEscape(const QString &seq);

    ConnSession *m_session = nullptr;
    QByteArray m_incomplete;     // 跨包残留的不完整转义序列
    QTextCharFormat m_fmt;       // 当前字符格式
    bool m_localEcho = false;    // 远端 PTY 回显，本地不回显
    QString m_encoding = "UTF-8"; // 当前编码（UTF-8/GBK/GB2312/BIG5/LATIN1）

signals:
    void sendData(const QByteArray &data);
};
