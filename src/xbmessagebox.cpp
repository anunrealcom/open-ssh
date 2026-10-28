#include "xbmessagebox.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>

static QIcon msgIcon(XbMessageBox::Style s)
{
    const int sz = 34;
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QColor bg = s == XbMessageBox::Warning ? QColor("#f0a020")
              : s == XbMessageBox::Critical ? QColor("#e81123")
              : QColor("#11a7ea");
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(1, 1, sz - 2, sz - 2);

    p.setPen(QPen(Qt::white, 2.6));
    p.setBrush(Qt::NoBrush);
    if (s == XbMessageBox::Info) {
        p.drawText(QRect(0, 0, sz, sz), Qt::AlignHCenter | Qt::AlignVCenter, "i");
    } else if (s == XbMessageBox::Question) {
        p.drawText(QRect(0, 0, sz, sz), Qt::AlignHCenter | Qt::AlignVCenter, "?");
    } else { // Warning / Critical: 感叹号
        p.drawLine(sz / 2, 9, sz / 2, 19);
        p.setBrush(Qt::white);
        p.drawEllipse(QPointF(sz / 2.0, 23.0), 1.6, 1.6);
    }
    p.end();
    return QIcon(pm);
}

XbMessageBox::XbMessageBox(QWidget *parent, const QString &title,
                           const QString &text, Style style, bool ask)
    : FramelessDialog(parent, title)
{
    auto *row = new QHBoxLayout();
    row->setSpacing(14);
    row->setContentsMargins(6, 6, 6, 6);

    auto *icon = new QLabel();
    icon->setPixmap(msgIcon(style).pixmap(34, 34));
    icon->setFixedSize(36, 36);
    icon->setAlignment(Qt::AlignTop);

    auto *txt = new QLabel(text);
    txt->setWordWrap(true);
    txt->setStyleSheet("QLabel{color:#2b3a4b;font-size:13px;line-height:1.5;}");
    txt->setMinimumWidth(260);
    txt->setMaximumWidth(440);

    row->addWidget(icon);
    row->addWidget(txt, 1);
    contentLayout()->addLayout(row);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    if (ask) {
        auto *no = new QPushButton("否");
        auto *yes = new QPushButton("是");
        yes->setObjectName("primaryBtn");
        connect(no, &QPushButton::clicked, this, &QDialog::reject);
        connect(yes, &QPushButton::clicked, this, &QDialog::accept);
        btnRow->addWidget(no);
        btnRow->addWidget(yes);
    } else {
        auto *ok = new QPushButton("确定");
        ok->setObjectName("primaryBtn");
        connect(ok, &QPushButton::clicked, this, &QDialog::accept);
        btnRow->addWidget(ok);
    }
    contentLayout()->addLayout(btnRow);

    resize(440, 170);
}

int XbMessageBox::information(QWidget *parent, const QString &title, const QString &text)
{
    XbMessageBox d(parent, title, text, Info, false);
    return d.exec();
}

int XbMessageBox::warning(QWidget *parent, const QString &title, const QString &text)
{
    XbMessageBox d(parent, title, text, Warning, false);
    return d.exec();
}

int XbMessageBox::critical(QWidget *parent, const QString &title, const QString &text)
{
    XbMessageBox d(parent, title, text, Critical, false);
    return d.exec();
}

int XbMessageBox::question(QWidget *parent, const QString &title, const QString &text)
{
    XbMessageBox d(parent, title, text, Question, true);
    return d.exec();
}
