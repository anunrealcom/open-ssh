#include "framelessdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

QIcon xbMakeIcon(const QString &kind, const QColor &fg, int size)
{
    const int s = size;
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal penW = qMax(1.2, s / 10.0);
    p.setPen(QPen(fg, penW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal m = s * 0.10;      // 边距（收紧让图案占满）
    const qreal L = s * 0.36;      // 线长基准
    const qreal x1 = m;
    const qreal y1 = m;
    const qreal x2 = s - m;
    const qreal y2 = s - m;
    const qreal cx = s / 2.0;
    const qreal cy = s / 2.0;

    if (kind == "logo") {
        // Logo：白色终端提示符 >_ （简约、零字体依赖）
        qreal dashW = L * 1.2;
        p.drawLine(QPointF(x1 + 2, y1 + 3), QPointF(x1 + 2 + dashW, cy));
        p.drawLine(QPointF(x1 + 2, y2 - 3), QPointF(x1 + 2 + dashW, cy));
        p.drawLine(QPointF(x1 + 2, y2 - 3), QPointF(x2 - 2, y2 - 3));
    } else if (kind == "min") {
        // 最小化：底边横线
        p.drawLine(QPointF(x1 + 1, y2 - 1), QPointF(x2 - 1, y2 - 1));
    } else if (kind == "max") {
        // 最大化：空心方块
        p.drawRect(QRectF(x1, y1, x2 - x1, y2 - y1));
    } else if (kind == "restore") {
        // 还原：重叠的两个方块
        qreal o = s * 0.22;
        p.drawRect(QRectF(x1 + o, y1, x2 - x1 - o, y2 - y1 - o));
        p.drawLine(QPointF(x1 + 1, y1 + o), QPointF(x1 + o, y1 + o));
        p.drawLine(QPointF(x1 + o, y1 + 1), QPointF(x1 + o, y1 + o));
        p.drawLine(QPointF(x1 + o, y1 + o), QPointF(x2 - 1, y1 + o));
        p.drawLine(QPointF(x2 - 1, y1 + o), QPointF(x2 - 1, y2 - o - 1));
    } else if (kind == "close") {
        // 关闭：叉
        p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        p.drawLine(QPointF(x2, y1), QPointF(x1, y2));
    } else if (kind == "newSession") {
        // 新建会话：文件页 + 加号
        p.drawRect(QRectF(x1, y1 + 2, x2 - x1 - 2, y2 - y1 - 2));
        p.drawLine(QPointF(cx, y1 + 5), QPointF(cx, y2 - 3));
        p.drawLine(QPointF(x1 + 4, cy), QPointF(x2 - 5, cy));
    } else if (kind == "quick") {
        // 快速连接：闪电
        QPolygonF poly;
        poly << QPointF(cx - 2, y1 + 2)
             << QPointF(cx + 3, cy - 1)
             << QPointF(cx, cy - 1)
             << QPointF(cx + 2, y2 - 2)
             << QPointF(cx - 3, cy + 1)
             << QPointF(cx, cy + 1);
        p.drawPolygon(poly);
    } else if (kind == "desktop") {
        // 远程桌面：显示器
        p.drawRect(QRectF(x1 + 1, y1 + 1, x2 - x1 - 2, (y2 - y1) * 0.62));
        p.drawLine(QPointF(cx, y1 + (y2 - y1) * 0.62 + 1), QPointF(cx, y2 - 1));
        p.drawLine(QPointF(cx - L + 2, y2 - 1), QPointF(cx + L - 2, y2 - 1));
    } else if (kind == "files") {
        // 文件传输：文件夹
        QPointF topLeft(x1, y1 + 3);
        QPointF topRight(x2, y1 + 3);
        QPointF bottomRight(x2, y2);
        QPointF bottomLeft(x1, y2);
        p.drawLine(topLeft, QPointF(x1 + 4, y1 + 3));
        p.drawLine(QPointF(x1 + 4, y1 + 3), QPointF(x1 + 7, y1));
        p.drawLine(QPointF(x1 + 7, y1), QPointF(x1 + 12, y1));
        p.drawLine(QPointF(x1 + 12, y1), topRight);
        p.drawLine(topRight, bottomRight);
        p.drawLine(bottomRight, bottomLeft);
        p.drawLine(bottomLeft, topLeft);
    } else if (kind == "tunnel") {
        // 端口转发：T 型管道
        p.drawLine(QPointF(x1, cy), QPointF(x2 - 4, cy));
        p.drawLine(QPointF(x2 - 4, cy), QPointF(x2 - 4, y1 + 3));
        p.drawLine(QPointF(x2 - 4, cy), QPointF(x2 - 4, y2 - 3));
        p.drawEllipse(QPointF(x1 + 2, cy), 1.3, 1.3);
        p.drawEllipse(QPointF(x2 - 4, y1 + 3), 1.3, 1.3);
        p.drawEllipse(QPointF(x2 - 4, y2 - 3), 1.3, 1.3);
    } else if (kind == "tunnels") {
        // 隧道管理：列表 + 管道
        for (int i = 0; i < 3; ++i)
            p.drawLine(QPointF(x1, y1 + 3 + i * 4), QPointF(x1 + 5, y1 + 3 + i * 4));
        p.drawLine(QPointF(x1 + 7, cy), QPointF(x2, cy));
        p.drawLine(QPointF(x1 + 7, cy), QPointF(x1 + 7, y1 + 3));
        p.drawLine(QPointF(x1 + 7, cy), QPointF(x1 + 7, y2 - 3));
        p.drawEllipse(QPointF(x1 + 7, y1 + 3), 1.0, 1.0);
        p.drawEllipse(QPointF(x1 + 7, y2 - 3), 1.0, 1.0);
    } else if (kind == "serial") {
        // 串口：D 型接口
        p.drawRect(QRectF(cx - 3, y1 + 2, 6, y2 - y1 - 4));
        p.drawLine(QPointF(cx - 3, y1 + 5), QPointF(cx + 3, y1 + 5));
        p.drawLine(QPointF(cx - 2, y2 - 3), QPointF(cx - 2, y2 - 1));
        p.drawLine(QPointF(cx,     y2 - 3), QPointF(cx,     y2 - 1));
        p.drawLine(QPointF(cx + 2, y2 - 3), QPointF(cx + 2, y2 - 1));
    } else if (kind == "ai") {
        // AI 助手：星/火花
        QPolygonF star;
        star << QPointF(cx, y1)
             << QPointF(cx + 2, cy - 2)
             << QPointF(x2, cy)
             << QPointF(cx + 2, cy + 2)
             << QPointF(cx, y2)
             << QPointF(cx - 2, cy + 2)
             << QPointF(x1, cy)
             << QPointF(cx - 2, cy - 2);
        p.drawPolygon(star);
    } else if (kind == "encoding") {
        // 编码：地球/字母 A
        p.drawEllipse(QRectF(cx - L, cy - L, L * 2, L * 2));
        p.drawLine(QPointF(cx, cy - L), QPointF(cx, cy + L));
        p.drawLine(QPointF(cx - L, cy), QPointF(cx + L, cy));
        QFont f = p.font();
        f.setPointSizeF(qMax(6.0, s * 0.45));
        f.setBold(true);
        p.setFont(f);
        p.setPen(fg);
        p.drawText(QRectF(0, 0, s, s), Qt::AlignCenter, "A");
    } else if (kind == "group") {
        // 分组：文件夹
        QPointF topLeft(x1, y1 + 3);
        QPointF topRight(x2, y1 + 3);
        QPointF bottomRight(x2, y2);
        QPointF bottomLeft(x1, y2);
        p.drawLine(topLeft, QPointF(x1 + 4, y1 + 3));
        p.drawLine(QPointF(x1 + 4, y1 + 3), QPointF(x1 + 7, y1));
        p.drawLine(QPointF(x1 + 7, y1), QPointF(x1 + 12, y1));
        p.drawLine(QPointF(x1 + 12, y1), topRight);
        p.drawLine(topRight, bottomRight);
        p.drawLine(bottomRight, bottomLeft);
        p.drawLine(bottomLeft, topLeft);
    } else if (kind == "linux") {
        // Linux 会话：终端提示符 >_ （带圆角框）
        p.drawRoundedRect(QRectF(x1, y1, x2 - x1, y2 - y1), L * 0.4, L * 0.4);
        qreal t = y2 - 4;
        p.drawLine(QPointF(x1 + 3, cy), QPointF(x1 + 3 + L, cy + L * 0.7));
        p.drawLine(QPointF(x1 + 3 + L, cy + L * 0.7), QPointF(x1 + 3, t));
        p.drawLine(QPointF(x1 + 3, t), QPointF(x2 - 3, t));
    } else if (kind == "windows") {
        // Windows 会话：四格窗口
        p.drawRect(QRectF(x1, y1, x2 - x1, y2 - y1));
        p.drawLine(QPointF(cx, y1), QPointF(cx, y2));
        p.drawLine(QPointF(x1, cy), QPointF(x2, cy));
    } else {
        // 默认：小圆点
        p.drawEllipse(QRectF(cx - 2, cy - 2, 4, 4));
    }
    p.end();
    return QIcon(pm);
}

FramelessDialog::FramelessDialog(QWidget *parent, const QString &title)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);

    m_container = new QWidget(this);
    m_container->setObjectName("dlgContainer");
    auto *v = new QVBoxLayout(m_container);
    v->setContentsMargins(12, 12, 12, 12);
    v->setSpacing(0);

    // 标题栏
    m_titleBar = new QWidget();
    m_titleBar->setObjectName("dlgTitleBar");
    m_titleBar->setFixedHeight(38);
    auto *hl = new QHBoxLayout(m_titleBar);
    hl->setContentsMargins(12, 0, 8, 0);
    m_titleLabel = new QLabel(title);
    m_titleLabel->setObjectName("dlgTitleLabel");
    hl->addWidget(m_titleLabel);
    hl->addStretch(1);

    m_closeBtn = new QPushButton();
    m_closeBtn->setObjectName("dlgCloseBtn");
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setIcon(xbMakeIcon("close"));
    m_closeBtn->setIconSize(QSize(14, 14));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    hl->addWidget(m_closeBtn);
    v->addWidget(m_titleBar);

    // 内容
    m_content = new QWidget();
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setContentsMargins(6, 10, 6, 6);
    m_contentLayout->setSpacing(10);
    v->addWidget(m_content, 1);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_container);

    auto *sh = new QGraphicsDropShadowEffect(this);
    sh->setBlurRadius(16);
    sh->setOffset(0, 3);
    sh->setColor(QColor(0, 0, 0, 55));
    m_container->setGraphicsEffect(sh);

    m_container->setStyleSheet(
        "QWidget#dlgContainer{background:#ffffff;border-radius:10px;}");
    m_titleBar->setStyleSheet(
        "QWidget#dlgTitleBar{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "stop:0 #2ec4f6,stop:1 #11a7ea);border-top-left-radius:10px;"
        "border-top-right-radius:10px;}"
        "QLabel#dlgTitleLabel{color:#ffffff;font-size:13px;font-weight:600;}");
    m_closeBtn->setStyleSheet(
        "QPushButton#dlgCloseBtn{background:transparent;border:none;"
        "border-radius:6px;padding:0;}"
        "QPushButton#dlgCloseBtn:hover{background:rgba(255,255,255,0.22);}");
}

QVBoxLayout *FramelessDialog::contentLayout()
{
    return m_contentLayout;
}

void FramelessDialog::setWindowTitle(const QString &t)
{
    QDialog::setWindowTitle(t);
    if (m_titleLabel)
        m_titleLabel->setText(t);
}

bool FramelessDialog::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCHITTEST) {
        QPoint p = mapFromGlobal(QPoint(static_cast<short>(LOWORD(msg->lParam)),
                                       static_cast<short>(HIWORD(msg->lParam))));
        QRect tr(m_titleBar->mapTo(this, QPoint(0, 0)), m_titleBar->size());
        QRect cr(m_closeBtn->mapTo(this, QPoint(0, 0)), m_closeBtn->size());
        if (tr.contains(p) && !cr.contains(p)) {
            *result = HTCAPTION;
            return true;
        }
        *result = HTCLIENT;
        return true;
    }
    return QDialog::nativeEvent(eventType, message, result);
}

void FramelessDialog::mouseDoubleClickEvent(QMouseEvent *e)
{
    QPoint p = e->pos();
    QRect tr(m_titleBar->mapTo(this, QPoint(0, 0)), m_titleBar->size());
    QRect cr(m_closeBtn->mapTo(this, QPoint(0, 0)), m_closeBtn->size());
    if (tr.contains(p) && !cr.contains(p))
        showNormal(); // 对话框双击标题栏还原（无最大化意义，仅避免异常）
    QDialog::mouseDoubleClickEvent(e);
}
