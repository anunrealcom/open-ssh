#include "mainwindow.h"
#include "framelessdialog.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QLinearGradient>
#include <QPainterPath>

/**
 * 动态生成应用 Logo（蓝渐变圆角 + 白色“运”字）。
 * 提供 16/24/32/48/64/128/256 多尺寸，任务栏/标题栏/Alt-Tab 都能清晰显示。
 */
static QIcon createAppIcon()
{
    QIcon icon;
    QList<int> sizes = {16, 24, 32, 48, 64, 128, 256};
    for (int s : sizes) {
        QPixmap pm(s, s);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);

        int r = qMax(3, s / 8);
        QPainterPath path;
        path.addRoundedRect(2, 2, s - 4, s - 4, r, r);

        QLinearGradient g(0, 0, 0, s);
        g.setColorAt(0, QColor("#2ec4f6"));
        g.setColorAt(1, QColor("#11a7ea"));
        p.setBrush(g);
        p.setPen(Qt::NoPen);
        p.drawPath(path);

        // 简约终端提示符 >_ （和标题栏 Logo 一致）
        QPixmap fg = xbMakeIcon("logo", Qt::white, s).pixmap(s, s);
        p.drawPixmap(0, 0, fg);
        p.end();
        icon.addPixmap(pm);
    }
    return icon;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("小白运维 xiaobaiOP");
    QApplication::setApplicationDisplayName("小白运维 xiaobaiOP");
    QApplication::setOrganizationName("xiaobai");
    QApplication::setWindowIcon(createAppIcon());

    // 终端需要等宽字体
    QFont f("Consolas", 10);
    if (!f.exactMatch()) f = QFont("Courier New", 10);
    f.setStyleHint(QFont::Monospace);
    QApplication::setFont(f);

    // 全局样式（QQ 风：浅色面板 + 圆角控件 + 蓝色强调）
    a.setStyleSheet(R"(
        QLineEdit, QSpinBox, QComboBox {
            border: 1px solid #cfd8e3; border-radius: 6px;
            padding: 5px 8px; background: #ffffff; min-height: 20px; color:#2b3a4b;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #11a7ea; }
        QComboBox QAbstractItemView { border: 1px solid #cfd8e3; selection-background-color: #d4ecff; }
        QSpinBox::up-button, QSpinBox::down-button { width: 16px; }
        QPushButton {
            background: #f2f6fb; border: 1px solid #cfd8e3; border-radius: 6px;
            padding: 6px 16px; color: #2b3a4b; font-size: 13px;
        }
        QPushButton:hover { background: #e3f0fb; color: #0b76c4; border-color: #9fd0f0; }
        QPushButton:pressed { background: #d4ecff; }
        QPushButton#primaryBtn {
            background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2ec4f6,stop:1 #11a7ea);
            color: #ffffff; border: none; font-weight: 600; padding: 6px 18px;
        }
        QPushButton#primaryBtn:hover { background: #11a7ea; color: #ffffff; }
        QPushButton#primaryBtn:disabled { background: #b6d8ec; color: #f0f8ff; }
        QTabWidget::pane { border: none; }
        QTabBar::tab {
            background: #eef3f8; color: #5b6b7c; padding: 7px 14px;
            border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px;
        }
        QTabBar::tab:selected { background: #ffffff; color: #0b76c4; font-weight: 600; }
        QListWidget { border: 1px solid #e3e8ef; border-radius: 6px; background: #ffffff; color:#2b3a4b; }
        QListWidget::item { padding: 4px; }
        QListWidget::item:selected { background: #d4ecff; color: #0b76c4; }
        QCheckBox { spacing: 6px; color: #2b3a4b; }
        QLabel { color: #2b3a4b; }
        QFormLayout QLabel { color: #5b6b7c; }
        QMessageBox { background: #ffffff; }
    )");

    MainWindow w;
    w.show();
    return a.exec();
}
