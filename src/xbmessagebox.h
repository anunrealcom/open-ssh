#pragma once

#include "framelessdialog.h"

/**
 * QQ 风格无边框提示框：圆角白底 + 彩色圆形图标 + 按钮。
 * 与 FramelessDialog 风格一致，替代系统 QMessageBox。
 * 返回值用 QDialog::Accepted / QDialog::Rejected 判断。
 */
class XbMessageBox : public FramelessDialog
{
    Q_OBJECT
public:
    enum Style { Info, Warning, Critical, Question };

    static int information(QWidget *parent, const QString &title, const QString &text);
    static int warning(QWidget *parent, const QString &title, const QString &text);
    static int critical(QWidget *parent, const QString &title, const QString &text);
    /** 是/否 询问，返回 Accepted(是) / Rejected(否) */
    static int question(QWidget *parent, const QString &title, const QString &text);

private:
    XbMessageBox(QWidget *parent, const QString &title, const QString &text,
                 Style style, bool ask);
};
