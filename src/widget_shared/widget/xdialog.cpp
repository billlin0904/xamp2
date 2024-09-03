#include <QCloseEvent>

#include <widget/xdialog.h>

#include <thememanager.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

#include <QLabel>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

XDialog::XDialog(QWidget* parent, bool modal)
    : QDialog(parent) {
    setModal(modal);
}

void XDialog::setContent(QWidget* content) {
    content_ = content;

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    setLayout(default_layout);

    default_layout->addWidget(content_, 1);
    default_layout->setContentsMargins(0, 0, 0, 0);

    // 重要! 避免出現setGeometry Unable to set geometry錯誤
    adjustSize();
}

void XDialog::onThemeChangedFinished(ThemeColor theme_color) {
}

void XDialog::setTitle(const QString& title) {
    setWindowTitle(title);
}

void XDialog::setIcon(const QIcon& icon) {
    setWindowIcon(icon);
}

void XDialog::setContentWidget(QWidget* content, bool no_moveable, bool disable_resize) {
    setContent(content);
}

void XDialog::closeEvent(QCloseEvent* event) {
    if (!content_) {
        return;
    }
    if (!content_->close()) {
        event->ignore();
        return;
    }
}
