#include "terminalwidget.h"

#include <QKeyEvent>
#include <QScrollBar>

#include "conn.h"

// 编码转换使用 Windows 原生 API（本机 Qt 未提供 QTextCodec / Core5Compat）
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
    UINT encodingToCP(const QString &enc)
    {
        QString e = enc.toUpper();
        if (e == "UTF-8")     return CP_UTF8;
        if (e == "GBK" || e == "GB2312") return 936;   // 代码页 936 = GBK/GB2312
        if (e == "BIG5")      return 950;
        if (e == "LATIN1")    return 28591;             // ISO-8859-1
        return CP_UTF8;
    }

    QString decodeBytes(const QByteArray &b, const QString &enc)
    {
        if (b.isEmpty())
            return QString();
        UINT cp = encodingToCP(enc);
        int n = MultiByteToWideChar(cp, 0, b.constData(), b.size(), nullptr, 0);
        if (n <= 0)
            return QString::fromUtf8(b);
        std::wstring w(static_cast<size_t>(n), L'\0');
        MultiByteToWideChar(cp, 0, b.constData(), b.size(), &w[0], n);
        return QString::fromWCharArray(w.c_str());
    }

    QByteArray encodeString(const QString &s, const QString &enc)
    {
        UINT cp = encodingToCP(enc);
        int n = WideCharToMultiByte(cp, 0, reinterpret_cast<LPCWSTR>(s.utf16()),
                                    s.length(), nullptr, 0, nullptr, nullptr);
        if (n <= 0)
            return s.toUtf8();
        QByteArray out(n, 0);
        WideCharToMultiByte(cp, 0, reinterpret_cast<LPCWSTR>(s.utf16()),
                            s.length(), out.data(), n, nullptr, nullptr);
        return out;
    }
}

TerminalWidget::TerminalWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setFont(QFont("Consolas", 10));
    setLineWrapMode(NoWrap);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // 暗色终端风格
    setStyleSheet("QPlainTextEdit{background-color:#1e1e1e;color:#d4d4d4;"
                 "selection-color:#1e1e1e;selection-background-color:#264f78;"
                 "border:0;}");
    m_fmt.setForeground(QColor("#d4d4d4"));
}

void TerminalWidget::setEncoding(const QString &name)
{
    if (!name.isEmpty())
        m_encoding = name;
}

QString TerminalWidget::encoding() const
{
    return m_encoding;
}

void TerminalWidget::setSession(ConnSession *session)
{
    m_session = session;
    if (m_session) {
        connect(m_session, &ConnSession::output,
                this, &TerminalWidget::appendOutput);
        connect(this, &TerminalWidget::sendData,
                m_session, &ConnSession::write);
    }
}

void TerminalWidget::insertText(const QString &text, const QTextCharFormat &fmt)
{
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertText(text, fmt);
    setTextCursor(cur);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::applySGR(const QStringList &codes)
{
    // 默认色
    auto reset = [this]() {
        m_fmt.setForeground(QColor("#d4d4d4"));
        m_fmt.setBackground(QColor("#1e1e1e"));
        m_fmt.setFontWeight(QFont::Normal);
    };
    if (codes.isEmpty() || (codes.size() == 1 && (codes[0].isEmpty() || codes[0] == "0"))) {
        reset();
        return;
    }
    for (const QString &c : codes) {
        int n = c.toInt();
        switch (n) {
        case 0: reset(); break;
        case 1: m_fmt.setFontWeight(QFont::Bold); break;
        case 30: m_fmt.setForeground(Qt::black); break;
        case 31: m_fmt.setForeground(Qt::red); break;
        case 32: m_fmt.setForeground(Qt::green); break;
        case 33: m_fmt.setForeground(Qt::yellow); break;
        case 34: m_fmt.setForeground(Qt::blue); break;
        case 35: m_fmt.setForeground(Qt::magenta); break;
        case 36: m_fmt.setForeground(Qt::cyan); break;
        case 37: m_fmt.setForeground(Qt::white); break;
        case 90: m_fmt.setForeground(QColor("#808080")); break;
        case 91: m_fmt.setForeground(QColor("#ff5555")); break;
        case 92: m_fmt.setForeground(QColor("#55ff55")); break;
        case 93: m_fmt.setForeground(QColor("#ffff55")); break;
        case 94: m_fmt.setForeground(QColor("#5555ff")); break;
        case 95: m_fmt.setForeground(QColor("#ff55ff")); break;
        case 96: m_fmt.setForeground(QColor("#55ffff")); break;
        case 39: m_fmt.setForeground(QColor("#d4d4d4")); break;
        case 40: m_fmt.setBackground(Qt::black); break;
        case 41: m_fmt.setBackground(Qt::red); break;
        case 42: m_fmt.setBackground(Qt::green); break;
        case 43: m_fmt.setBackground(Qt::yellow); break;
        case 44: m_fmt.setBackground(Qt::blue); break;
        case 45: m_fmt.setBackground(Qt::magenta); break;
        case 46: m_fmt.setBackground(Qt::cyan); break;
        case 47: m_fmt.setBackground(Qt::white); break;
        case 49: m_fmt.setBackground(QColor("#1e1e1e")); break;
        default: break;
        }
    }
}

void TerminalWidget::handleEscape(const QString &seq)
{
    // seq 形如 "\x1b[...m" 或 "\x1b[K" 等
    if (!seq.startsWith("\x1b["))
        return;
    QString body = seq.mid(2);
    if (body.isEmpty())
        return;
    QChar final = body.back();
    if (final == 'm') {
        QStringList nums = body.left(body.size() - 1).split(';', Qt::SkipEmptyParts);
        applySGR(nums);
    }
    // 其它（K/J/A/B/C/D/H/f 等）按无副作用消费，保持 shell 可用
}

void TerminalWidget::appendOutput(const QByteArray &data)
{
    QByteArray buf = m_incomplete + data;
    int i = 0;
    const int n = buf.size();
    QByteArray textBuf;  // 累积待解码文本（多字节编码需整段交给 codec）
    auto flushText = [this, &textBuf]() {
        if (!textBuf.isEmpty()) {
            insertText(decodeBytes(textBuf, m_encoding), m_fmt);
            textBuf.clear();
        }
    };
    while (i < n) {
        char c = buf.at(i);
        if (c == 27) { // ESC
            flushText();
            int j = i + 1;
            bool ok = false;
            while (j < n) {
                char e = buf.at(j);
                if ((e >= 'A' && e <= 'Z') || (e >= 'a' && e <= 'z')) {
                    ok = true;
                    break;
                }
                ++j;
            }
            if (!ok) {
                m_incomplete = buf.mid(i); // 等待后续字节
                return;
            }
            handleEscape(QString::fromLatin1(buf.mid(i, j - i + 1)));
            i = j + 1;
            continue;
        }
        if (c == '\r') {
            flushText();
            if (i + 1 < n && buf.at(i + 1) == '\n') {
                insertText("\n", m_fmt);
                i += 2;
                continue;
            }
            ++i; // 单独 \r 忽略
            continue;
        }
        if (c == '\b') {
            flushText();
            QTextCursor cur = textCursor();
            cur.movePosition(QTextCursor::End);
            cur.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            cur.removeSelectedText();
            setTextCursor(cur);
            ++i;
            continue;
        }
        if (c == '\n') {
            flushText();
            insertText("\n", m_fmt);
            ++i;
            continue;
        }
        if (c == '\t') {
            flushText();
            insertText("    ", m_fmt);
            ++i;
            continue;
        }
        if (static_cast<unsigned char>(c) < 0x20) {
            flushText();
            ++i; // 其它控制字符忽略
            continue;
        }
        // 可打印字符（含多字节 UTF-8 / GBK 的后续字节，统一累积后整段解码）
        textBuf.append(c);
        ++i;
    }
    flushText();
    m_incomplete.clear();
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent *e)
{
    if (m_session && !e->commitString().isEmpty()) {
        // 中文/日文等输入法提交：直接发送编码后的字节，依赖远端回显
        emit sendData(encodeString(e->commitString(), m_encoding));
        e->accept();
        return;
    }
    // 组合预览阶段不插入本地，避免与远端回显重复
    e->accept();
}

void TerminalWidget::keyPressEvent(QKeyEvent *e)
{
    if (!m_session) {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }

    // 功能 / 方向键
    switch (e->key()) {
    case Qt::Key_Backspace:
        emit sendData("\x7f");
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit sendData("\r");
        return;
    case Qt::Key_Tab:
        emit sendData("\t");
        return;
    case Qt::Key_Escape:
        emit sendData("\x1b");
        return;
    case Qt::Key_Up:
        emit sendData("\x1b[A"); return;
    case Qt::Key_Down:
        emit sendData("\x1b[B"); return;
    case Qt::Key_Right:
        emit sendData("\x1b[C"); return;
    case Qt::Key_Left:
        emit sendData("\x1b[D"); return;
    case Qt::Key_Home:
        emit sendData("\x1b[H"); return;
    case Qt::Key_End:
        emit sendData("\x1b[F"); return;
    case Qt::Key_PageUp:
        emit sendData("\x1b[5~"); return;
    case Qt::Key_PageDown:
        emit sendData("\x1b[6~"); return;
    case Qt::Key_Delete:
        emit sendData("\x1b[3~"); return;
    default:
        break;
    }

    // Ctrl + 字母
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z) {
            char ch = static_cast<char>(e->key() - Qt::Key_A + 1);
            emit sendData(QByteArray(1, ch));
            return;
        }
    }

    // 普通字符（按当前编码发送给对端）
    if (!e->text().isEmpty()) {
        emit sendData(encodeString(e->text(), m_encoding));
        return;
    }

    QPlainTextEdit::keyPressEvent(e);
}
