#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>

#include <widget/str_utilts.h>
#include <widget/xampdialog.h>

XampDialog::XampDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    centerWidgets(this);
    setFont(QFont(Q_UTF8("UI")));
}

void XampDialog::centerWidgets(QWidget* widget) {
    auto screens = QApplication::screens();
    auto desktop = QApplication::desktop();
    auto screen_num = desktop->screenNumber(QCursor::pos());
    QRect rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}