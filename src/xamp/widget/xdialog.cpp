#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScreen>
#include <QToolButton>
#include <QWindow>
#include <QMouseEvent>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <widget/win32/win32.h>
#endif

#include <QGraphicsDropShadowEffect>
#include <windowsx.h>

#include "thememanager.h"
#include <widget/str_utilts.h>
#include <widget/xdialog.h>

XDialog::XDialog(QWidget* parent)
    : QDialog(parent) {
}

void XDialog::setContentWidget(QWidget* content) {
    content_widget_ = content;

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | windowFlags());
    //setAttribute(Qt::WA_TranslucentBackground, true);

    auto* default_layout = new QGridLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(default_layout);

    auto* titleFrame = new QFrame(this);
    titleFrame->setObjectName(QString::fromUtf8("titleFrame"));
    titleFrame->setMinimumSize(QSize(0, 24));
    titleFrame->setFrameShape(QFrame::NoFrame);
    titleFrame->setFrameShadow(QFrame::Plain);
    //titleFrame->setLineWidth(1);
    titleFrame->setStyleSheet(Q_UTF8("QFrame#titleFrame { border: none; }"));

    auto* titleFrameLabel = new QLabel(titleFrame);
    titleFrameLabel->setObjectName(QString::fromUtf8("titleFrameLabel"));
    QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy3.setHorizontalStretch(1);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(titleFrameLabel->sizePolicy().hasHeightForWidth());
    titleFrameLabel->setSizePolicy(sizePolicy3);

    auto* closeButton = new QToolButton(titleFrame);
    closeButton->setObjectName(QString::fromUtf8("closeButton"));
    closeButton->setMinimumSize(QSize(24, 24));
    closeButton->setMaximumSize(QSize(24, 24));
    closeButton->setFocusPolicy(Qt::NoFocus);

    auto* maxWinButton = new QToolButton(titleFrame);
    maxWinButton->setObjectName(QString::fromUtf8("maxWinButton"));
    maxWinButton->setMinimumSize(QSize(24, 24));
    maxWinButton->setMaximumSize(QSize(24, 24));
    maxWinButton->setFocusPolicy(Qt::NoFocus);

    auto* minWinButton = new QToolButton(titleFrame);
    minWinButton->setObjectName(QString::fromUtf8("minWinButton"));
    minWinButton->setMinimumSize(QSize(24, 24));
    minWinButton->setMaximumSize(QSize(24, 24));
    minWinButton->setFocusPolicy(Qt::NoFocus);
    minWinButton->setPopupMode(QToolButton::InstantPopup);

    auto* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* horizontalLayout = new QHBoxLayout(titleFrame);
    horizontalLayout->addItem(horizontalSpacer);
    horizontalLayout->addWidget(titleFrameLabel);
    horizontalLayout->addWidget(minWinButton);
    horizontalLayout->addWidget(maxWinButton);
    horizontalLayout->addWidget(closeButton);

    horizontalLayout->setSpacing(0);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->setContentsMargins(0, 0, 0, 0);

    default_layout->addWidget(titleFrame, 0, 1, 1, 3);

#if defined(Q_OS_WIN)
    //win32::setFramelessWindowStyle(this);
    //win32::drawDwmShadow(this);
#endif

    /*auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setOffset(0, 0);
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(20);
    content_widget_->setGraphicsEffect(shadow);*/

	default_layout->addWidget(content_widget_, 2, 2, 1, 1);

    closeButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         image: url(:/xamp/Resource/%1/close.png);
                                         background-color: transparent;
                                         }
                                         )").arg(ThemeManager::instance().themeColorPath()));

    minWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/minimize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(ThemeManager::instance().themeColorPath()));

    maxWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/maximize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(ThemeManager::instance().themeColorPath()));

    maxWinButton->setDisabled(true);
    minWinButton->setDisabled(true);
    centerWidgets(this);
    setMouseTracking(true);

    (void)QObject::connect(closeButton, &QToolButton::pressed, [this]() {
        QWidget::close();
        });

    setContentsMargins(0, 0, 20, 20);
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

    if (!content_widget_) {
        return;
    }

    if (current_screen_ == nullptr) {
        current_screen_ = content_widget_->window()->windowHandle()->screen();
    }
    else if (current_screen_ != content_widget_->window()->windowHandle()->screen()) {
        current_screen_ = content_widget_->window()->windowHandle()->screen();

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

void XDialog::centerWidgets(QWidget* widget) {
    auto desktop = QApplication::desktop();
    auto screen_num = desktop->screenNumber(QCursor::pos());
    QRect rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}

void XDialog::centerParent() {
    if (this->parent() && this->parent()->isWidgetType()) {
        move((parentWidget()->width() - width()) / 2,
            (parentWidget()->height() - height()) / 2);
    }
}