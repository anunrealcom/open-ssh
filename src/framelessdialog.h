#pragma once

#include <QDialog>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

/**
 * 手绘图标（白底，适配深色标题栏）。kind:
 *   窗口控制: "min"/"max"/"close"/"restore"
 *   工具栏: "newSession"/"quick"/"desktop"/"files"/"tunnel"/"tunnels"/"serial"/"ai"/"encoding"
 * 用 QPainter 直接绘制，避免依赖特定字体导致字符渲染失败。
 */
QIcon xbMakeIcon(const QString &kind, const QColor &fg = Qt::white, int size = 16);

/**
 * QQ 风格无边框圆角对话框基类：
 * 自绘蓝色标题栏（含关闭按钮）+ 圆角白色容器 + 阴影。
 * 子类把内容加到 contentLayout() 即可；setWindowTitle() 会同步更新标题栏文字。
 */
class FramelessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FramelessDialog(QWidget *parent = nullptr, const QString &title = QString());

    QVBoxLayout *contentLayout();   // 内容布局（已带内边距，子类直接 addLayout/addWidget）

    void setWindowTitle(const QString &t);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
    QWidget *m_container = nullptr;
    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_content = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
};
