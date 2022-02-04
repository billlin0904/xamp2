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
    default_layout->setContentsMargins(50, 50, 50, 50);
    setLayout(default_layout);

    auto* shadow = new QGraphicsDropShadowEffect(frame_);
    shadow->setOffset(0, 0);
    shadow->setBlurRadius(50);
    frame_->setGraphicsEffect(shadow);

	default_layout->addWidget(frame_, 2, 2, 1, 2);

    setMouseTracking(true);

    (void)QObject::connect(frame_, &XFrame::closeFrame, [this]() {
        QDialog::close();
        });
}

#if defined(Q_OS_WIN)
void XDialog::mousePressEvent(QMouseEvent* event) {
    if (ThemeManager::instance().useNativeWindow()) {
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
    if (ThemeManager::instance().useNativeWindow()) {
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
    if (ThemeManager::instance().useNativeWindow()) {
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

bool XDialog::hitTest(MSG const* msg, long* result) const {
    RECT winrect{};
    GetWindowRect(reinterpret_cast<HWND>(winId()), &winrect);

    const auto x = GET_X_LPARAM(msg->lParam);
    const auto y = GET_Y_LPARAM(msg->lParam);

    const auto left_range = winrect.left + border_width_;
    const auto right_range = winrect.right - border_width_;
    const auto top_range = winrect.top + border_width_;
    const auto bottom_range = winrect.bottom - border_width_;

    const auto fixed_width = minimumWidth() == maximumWidth();
    const auto fixed_height = minimumHeight() == maximumHeight();

    if (!fixed_width && !fixed_height) {
        // Bottom left corner
        if (x >= winrect.left && x < left_range &&
            y < winrect.bottom && y >= bottom_range) {
            *result = HTBOTTOMLEFT;
            return true;
        }

        // Bottom right corner
        if (x < winrect.right && x >= right_range &&
            y < winrect.bottom && y >= bottom_range) {
            *result = HTBOTTOMRIGHT;
            return true;
        }

        // Top left corner
        if (x >= winrect.left && x < left_range &&
            y >= winrect.top && y < top_range) {
            *result = HTTOPLEFT;
            return true;
        }

        // Top right corner
        if (x < winrect.right && x >= right_range &&
            y >= winrect.top && y < top_range) {
            *result = HTTOPRIGHT;
            return true;
        }
    }

    if (!fixed_width) {
        // Left border
        if (x >= winrect.left && x < left_range) {
            *result = HTLEFT;
            return true;
        }
        // Right border
        if (x < winrect.right && x >= right_range) {
            *result = HTRIGHT;
            return true;
        }
    }

    if (!fixed_height) {
        // Bottom border
        if (y < winrect.bottom && y >= bottom_range) {
            *result = HTBOTTOM;
            return true;
        }
        // Top border
        if (y >= winrect.top && y < top_range) {
            *result = HTTOP;
            return true;
        }
    }

    if (*result == HTCAPTION) {
        return false;
    }
    return true;
}
#endif

bool XDialog::nativeEvent(const QByteArray& event_type, void* message, long* result) {
    if (ThemeManager::instance().useNativeWindow()) {
        return QWidget::nativeEvent(event_type, message, result);
    }

#if defined(Q_OS_WIN)
    const auto* msg = static_cast<MSG const*>(message);
    switch (msg->message) {
    /*case WM_PAINT:
	    win32::drawGdiShadow(message);
        return true;
    case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
            win32::removeStandardFrame(message);
            return true;
        }
        return false;
    case WM_NCHITTEST:
        win32::setResizeable(this);
        return true;*/
    case WM_NCCALCSIZE:
        if (msg->wParam == FALSE) {
            *result = 0;
            return true;
        }
        *result = WVR_REDRAW;
        break;
    /*
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = HTCAPTION;
            return hitTest(msg, result);
        }
        return true;*/
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
#else
    return QWidget::nativeEvent(event_type, message, result);
#endif
}

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

void XDialog::closeEvent(QCloseEvent* event) {
    auto* opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    auto* opacityAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
    opacityAnimation->setStartValue(1);
    opacityAnimation->setEndValue(0);
    opacityAnimation->setDuration(100);
    opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
    (void)QObject::connect(opacityAnimation,
        &QPropertyAnimation::finished,
        this,
        &XDialog::deleteLater);
    opacityAnimation->start();    
    event->ignore();
}
