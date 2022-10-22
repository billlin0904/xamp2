#include <QApplication>
#include <QGridLayout>
#include <QScreen>
#include <QWindow>
#include <QMouseEvent>

#if defined(Q_OS_WIN)
#include <windowsx.h>
#include <widget/win32/win32.h>
#include <base/platfrom_handle.h>
#include <Dbt.h>
#else
#include <widget/osx/osx.h>
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

    if (!qTheme.useNativeWindow()) {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground, true);
    } else {
        setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
#ifdef Q_OS_WIN
        if (qTheme.themeColor() == ThemeColor::DARK_THEME) {
            win32::setTitleBarColor(winId(), qTheme.backgroundColor());
        }
        win32::addDwmShadow(winId());
#endif
    }

    auto* default_layout = new QGridLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(20, 20, 20, 20);
    setLayout(default_layout);

#ifdef Q_OS_WIN
    if (!qTheme.useNativeWindow()) {
        auto* shadow = new QGraphicsDropShadowEffect(frame_);
        shadow->setOffset(0, 0);
        shadow->setBlurRadius(20);

        switch (qTheme.themeColor()) {
        case ThemeColor::DARK_THEME:
            shadow->setColor(Qt::black);
            break;
        case ThemeColor::LIGHT_THEME:
            shadow->setColor(Qt::gray);
            break;
        }
        frame_->setGraphicsEffect(shadow);
    }
#endif
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
    if (!qTheme.useNativeWindow()) {
        auto* opacity_effect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(opacity_effect);
        auto* opacity_animation = new QPropertyAnimation(opacity_effect, "opacity", this);
        opacity_animation->setStartValue(0);
        opacity_animation->setEndValue(1);
        opacity_animation->setDuration(200);
        opacity_animation->setEasingCurve(QEasingCurve::InSine);
        (void)QObject::connect(opacity_animation,
            &QPropertyAnimation::finished,
            opacity_effect,
            &QGraphicsOpacityEffect::deleteLater);
        opacity_animation->start();
    }
    
    QDialog::showEvent(event);
}
