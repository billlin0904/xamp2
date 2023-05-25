#include <widget/xdialog.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>

#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

XDialog::XDialog(QWidget* parent)
    : FramelessDialog(parent) {
}

void XDialog::SetContentWidget(QWidget* content, bool transparent_frame, bool disable_resize) {
    frame_ = new XFrame(this);
    frame_->SetContentWidget(content);

    if (transparent_frame) {
        frame_->setStyleSheet(qTEXT("background: transparent; border: none;"));
    }

    if (disable_resize) {
        setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
    }

    auto* default_layout = new QGridLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(default_layout);

	default_layout->addWidget(frame_, 2, 2, 1, 2);
    setMouseTracking(true);
    (void)QObject::connect(frame_, &XFrame::CloseFrame, [this]() {
        QDialog::close();
        });
}

void XDialog::SetIcon(const QIcon& icon) const {
    frame_->SetIcon(icon);
}

void XDialog::showEvent(QShowEvent* event) {
    auto* opacity_effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacity_effect);
    auto* opacity_animation = new QPropertyAnimation(opacity_effect, "opacity", this);
    opacity_animation->setStartValue(0);
    opacity_animation->setEndValue(1);
    opacity_animation->setDuration(200);
    opacity_animation->setEasingCurve(QEasingCurve::OutCubic);
    (void)QObject::connect(opacity_animation,
        &QPropertyAnimation::finished,
        opacity_effect,
        &QGraphicsOpacityEffect::deleteLater);
    opacity_animation->start();

    setAttribute(Qt::WA_Mapped);
    QDialog::showEvent(event);
}
