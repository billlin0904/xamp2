#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>

#include <widget/str_utilts.h>
#include <widget/xampdialog.h>

XampDialog::XampDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    centerWidgets(this);    
}

void XampDialog::centerWidgets(QWidget* widget) {
    auto desktop = QApplication::desktop();
    auto screen_num = desktop->screenNumber(QCursor::pos());
    QRect rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}

void XampDialog::centerParent() {
    if (this->parent() && this->parent()->isWidgetType()) {
        move((parentWidget()->width() - width()) / 2,
            (parentWidget()->height() - height()) / 2);
    }
}