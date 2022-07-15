#include <QApplication>
#include <QGridLayout>
#include <QScreen>
#include <QWindow>
#include <QMouseEvent>

#if defined(Q_OS_WIN)
#include <windowsx.h>
#include <Windows.h>
#endif

#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

#include "thememanager.h"
#include <widget/xdialog.h>

XDialog::XDialog(QWidget* parent)
    : QDialog(parent) {
}

void XDialog::setContentWidget(QWidget* content) {
    frame_ = new XFrame(this);
    frame_->setContentWidget(content);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    auto* default_layout = new QGridLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(20, 20, 20, 20);
    setLayout(default_layout);

    auto* shadow = new QGraphicsDropShadowEffect(frame_);
    shadow->setOffset(0, 0);
    shadow->setBlurRadius(20);
    shadow->setColor(Qt::black);
    frame_->setGraphicsEffect(shadow);

	default_layout->addWidget(frame_, 2, 2, 1, 2);

    setMouseTracking(true);

    (void)QObject::connect(frame_, &XFrame::closeFrame, [this]() {
        QDialog::close();
        });
}

#if defined(Q_OS_WIN)
void XDialog::mousePressEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
#if defined(Q_OS_WIN)    
    last_pos_ = event->globalPos() - pos();
#else
    QWidget::mousePressEvent(event);
#endif
}

void XDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
#if defined(Q_OS_WIN)
    last_pos_ = QPoint();
#else
    QWidget::mouseReleaseEvent(event);
#endif
}

void XDialog::mouseMoveEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    if (qTheme.useNativeWindow()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (!last_pos_.isNull()) {
        move(event->globalPos() - last_pos_);
    }

    if (!frame_) {
        return;
    }

    if (current_screen_ == nullptr) {
        current_screen_ = frame_->window()->windowHandle()->screen();
    }
    else if (current_screen_ != frame_->window()->windowHandle()->screen()) {
        current_screen_ = frame_->window()->windowHandle()->screen();

        ::SetWindowPos(reinterpret_cast<HWND>(winId()), nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }
#else
    QWidget::mouseMoveEvent(event);
#endif   
}
#endif

void XDialog::showEvent(QShowEvent* event) {
    auto *opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    auto* opacityAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
    opacityAnimation->setStartValue(0);
    opacityAnimation->setEndValue(1);
    opacityAnimation->setDuration(200);
    opacityAnimation->setEasingCurve(QEasingCurve::InSine);
    (void)QObject::connect(opacityAnimation,
        &QPropertyAnimation::finished, 
        opacityEffect, 
        &QGraphicsOpacityEffect::deleteLater);
    opacityAnimation->start();
    QDialog::showEvent(event);
}
