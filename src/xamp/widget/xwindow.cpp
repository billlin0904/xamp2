#include <QApplication>
#include <QLayout>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>
#include <QStyleOption>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QIcon>
#include <QGraphicsDropShadowEffect>

#include "thememanager.h"

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <windowsx.h>
#include <QtWin>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>
#include <widget/win32/win32.h>
#else
#include <widget/osx/osx.h>
#endif

#include <widget/image_utiltis.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/xframe.h>
#include <widget/xwindow.h>

#if defined(Q_OS_WIN)
struct XWindow::WinTaskbar {
    WinTaskbar(XWindow *window, IXampPlayer* content_widget) {
        window_ = window;
        // TODO: not use standard icon?
        play_icon = window->style()->standardIcon(QStyle::SP_MediaPlay);
        pause_icon = window->style()->standardIcon(QStyle::SP_MediaPause);
        stop_play_icon = window->style()->standardIcon(QStyle::SP_MediaStop);
        seek_forward_icon = window->style()->standardIcon(QStyle::SP_MediaSeekForward);
        seek_backward_icon = window->style()->standardIcon(QStyle::SP_MediaSeekBackward);

        thumbnail_tool_bar_.reset(new QWinThumbnailToolBar(window));
        thumbnail_tool_bar_->setWindow(window->windowHandle());

        taskbar_button_.reset(new QWinTaskbarButton(window));
        taskbar_button_->setWindow(window->windowHandle());
        taskbar_progress_ = taskbar_button_->progress();
        taskbar_progress_->setVisible(true);

        auto* play_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
        play_tool_button->setIcon(play_icon);
        (void)QObject::connect(play_tool_button,
            &QWinThumbnailToolButton::clicked,
            content_widget,
            &IXampPlayer::play);

        auto* forward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
        forward_tool_button->setIcon(seek_forward_icon);
        (void)QObject::connect(forward_tool_button,
            &QWinThumbnailToolButton::clicked,
            content_widget,
            &IXampPlayer::playNextClicked);

        auto* backward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
        backward_tool_button->setIcon(seek_backward_icon);
        (void)QObject::connect(backward_tool_button,
            &QWinThumbnailToolButton::clicked,
            content_widget,
            &IXampPlayer::playPreviousClicked);

        thumbnail_tool_bar_->addButton(backward_tool_button);
        thumbnail_tool_bar_->addButton(play_tool_button);
        thumbnail_tool_bar_->addButton(forward_tool_button);
    }

    void setTaskbarProgress(const int32_t percent) {
        taskbar_progress_->setValue(percent);
    }

    void resetTaskbarProgress() {
        taskbar_progress_->reset();
        taskbar_progress_->setValue(0);
        taskbar_progress_->setRange(0, 100);
        taskbar_button_->setOverlayIcon(play_icon);
        taskbar_progress_->show();
    }

    void setTaskbarPlayingResume() {
        taskbar_button_->setOverlayIcon(play_icon);
        taskbar_progress_->resume();
    }

    void setTaskbarPlayerPaused() {
        taskbar_button_->setOverlayIcon(pause_icon);
        taskbar_progress_->pause();
    }

    void setTaskbarPlayerPlaying() {
        resetTaskbarProgress();
    }

    void setTaskbarPlayerStop() {
        taskbar_button_->setOverlayIcon(stop_play_icon);
        taskbar_progress_->hide();
    }

    void showEvent() {
        taskbar_button_->setWindow(window_->windowHandle());
    }

    QIcon play_icon;
    QIcon pause_icon;
    QIcon stop_play_icon;
    QIcon seek_forward_icon;
    QIcon seek_backward_icon;

private:
    XWindow* window_;
    QScopedPointer<QWinThumbnailToolBar> thumbnail_tool_bar_;
    QScopedPointer<QWinTaskbarButton> taskbar_button_;
    QWinTaskbarProgress* taskbar_progress_;
};

static XAMP_ALWAYS_INLINE LRESULT hitTest(HWND hwnd, MSG const* msg) noexcept {
    const POINT border{
        ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
        ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
    };
    const POINT cursor{ GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };

    RECT window;
    if (!::GetWindowRect(hwnd, &window)) {
        return HTNOWHERE;
    }

    constexpr auto borderless_drag = true;
    constexpr auto borderless_resize = true;

    const auto drag = borderless_drag ? HTCAPTION : HTCLIENT;

    enum RegionMask {
        CLIENT_MASK = 0b0000,
        LEFT_MASK = 0b0001,
        RIGHT_MASK = 0b0010,
        TOP_MASK = 0b0100,
        BOTTOM_MASK = 0b1000,
    };

    const auto result =
        LEFT_MASK * (cursor.x < (window.left + border.x)) |
        RIGHT_MASK * (cursor.x >= (window.right - border.x)) |
        TOP_MASK * (cursor.y < (window.top + border.y)) |
        BOTTOM_MASK * (cursor.y >= (window.bottom - border.y));

    switch (result) {
    case LEFT_MASK: return borderless_resize ? HTLEFT : drag;
    case RIGHT_MASK: return borderless_resize ? HTRIGHT : drag;
    case TOP_MASK: return borderless_resize ? HTTOP : drag;
    case BOTTOM_MASK: return borderless_resize ? HTBOTTOM : drag;
    case TOP_MASK | LEFT_MASK: return borderless_resize ? HTTOPLEFT : drag;
    case TOP_MASK | RIGHT_MASK: return borderless_resize ? HTTOPRIGHT : drag;
    case BOTTOM_MASK | LEFT_MASK: return borderless_resize ? HTBOTTOMLEFT : drag;
    case BOTTOM_MASK | RIGHT_MASK: return borderless_resize ? HTBOTTOMRIGHT : drag;
    case CLIENT_MASK: return drag;
    default: return HTNOWHERE;
    }
}
#endif

XWindow::XWindow()
    : IXWindow()
#if defined(Q_OS_WIN)
    , current_screen_(nullptr)
#endif
	, content_widget_(nullptr) {
    setObjectName(Q_TEXT("framelessWindow"));
}

void XWindow::setContentWidget(IXampPlayer *content_widget) {
    content_widget_ = content_widget;
    if (content_widget_ != nullptr) {
        /*auto* shadow = new QGraphicsDropShadowEffect();
        shadow->setOffset(0, 0);
        shadow->setBlurRadius(10);
        shadow->setColor(Qt::black);
        content_widget->setGraphicsEffect(shadow);*/
        auto* default_layout = new QGridLayout(this);
        default_layout->addWidget(content_widget_, 0, 0);
        qTheme.setBorderRadius(content_widget_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }

#if defined(Q_OS_WIN)
    if (!qTheme.useNativeWindow()) {
        setWindowTitle(kAppTitle);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
        win32::setFramelessWindowStyle(this);
        win32::addDwmShadow(this);
        if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
            content_widget->setAttribute(Qt::WA_TranslucentBackground, true);
            qTheme.enableBlur(this);
        }
    } else {
        win32::setWindowedWindowStyle(this);
        win32::addDwmShadow(this);
    }
    taskbar_.reset(new WinTaskbar(this, content_widget));
#else
    if (!qTheme.useNativeWindow()) {
        if (content_widget_ != nullptr) {
            osx::hideTitleBar(content_widget_);
        }
        setWindowTitle(kAppTitle);
    }
#endif

    setAcceptDrops(true);
    setMouseTracking(true);
    installEventFilter(this);
}

// QScopedPointer require default destructor.
XWindow::~XWindow() = default;

void XWindow::setTaskbarProgress(const int32_t percent) {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarProgress(percent);
#else
    (void)percent;
#endif
}

void XWindow::resetTaskbarProgress() {
#if defined(Q_OS_WIN)
    taskbar_->resetTaskbarProgress();
#endif
}

void XWindow::setTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayingResume();
#endif
}

void XWindow::setTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayerPaused();
#endif
}

void XWindow::setTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayerPlaying();
#endif
}

void XWindow::setTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayerStop();
#endif
}

bool XWindow::eventFilter(QObject * object, QEvent * event) {
    if (event->type() == QEvent::KeyPress) {
        const auto* key_event = dynamic_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            content_widget_->deleteKeyPress();
        }
    }
    return QWidget::eventFilter(object, event);
}

void XWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void XWindow::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void XWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void XWindow::dropEvent(QDropEvent* event) {
    const auto* mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        for (auto const& url : mime_data->urls()) {
            content_widget_->addDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

bool XWindow::nativeEvent(const QByteArray& event_type, void * message, long * result) {
    if (qTheme.useNativeWindow()) {
        return QWidget::nativeEvent(event_type, message, result);
    }

#if defined(Q_OS_WIN)
#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif
    const auto* msg = static_cast<MSG const*>(message);

    switch (msg->message) {
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = hitTest(reinterpret_cast<HWND>(winId()), msg);
            if (*result == HTCAPTION) {
                return false;
            }
            return true;
        }
        break;
    case WM_GETMINMAXINFO:
        if (layout() != nullptr) {
            if (win32::isWindowMaximized(this)) {
                RECT frame = { 0, 0, 0, 0 };
                ::AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
                frame.left = abs(frame.left);
                frame.top = abs(frame.bottom);
                layout()->setContentsMargins(frame.left, frame.top, frame.right, frame.bottom);
            }
            else {
                layout()->setContentsMargins(0, 0, 0, 0);
            }
        }            
        *result = ::DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        break;
    case WM_NCCALCSIZE:
        // this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
        if (msg->wParam == FALSE) {
            // 如果 wParam 為 FALSE，則 lParam 指向一個 RECT 結構。輸入時，該結構包含建議的視窗矩形視窗。退出時，該結構應包含相應視窗客戶區的螢屏坐標。
            *result = 0;
            return true;
        } else {
            // 如果 wParam 為 TRUE，lParam 指向一個 NCCALCSIZE_PARAMS 結構，該結構包含應用程式可以用來計算客戶矩形的新大小和位置的資訊。
            auto* nccalcsize_params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg->lParam);
            nccalcsize_params->rgrc[2] = nccalcsize_params->rgrc[1];
            nccalcsize_params->rgrc[1] = nccalcsize_params->rgrc[0];
            *result = WVR_REDRAW;
        }
        return true;
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME:
        // These undocumented messages are sent to draw themed window
        // borders. Block them to prevent drawing borders over the client
        // area.
        *result = 0;
        return true;
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
#else
    return QWidget::nativeEvent(event_type, message, result);
#endif
}

void XWindow::changeEvent(QEvent* event) {
#if defined(Q_OS_MAC)
    if (!qTheme.useNativeWindow() && content_widget_ != nullptr) {
        osx::hideTitleBar(content_widget_);
	}
#endif
    QWidget::changeEvent(event);
}

void XWindow::closeEvent(QCloseEvent* event) {
    if (content_widget_ != nullptr) {
        content_widget_->close();
    }
    QWidget::closeEvent(event);
}

void XWindow::mousePressEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mousePressEvent(event);
        return;
	}
    if (isMaximized()) {
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

void XWindow::mouseReleaseEvent(QMouseEvent* event) {
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

void XWindow::mouseDoubleClickEvent(QMouseEvent*) {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void XWindow::mouseMoveEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

#if defined(Q_OS_WIN)
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
#ifdef Q_OS_WIN32
void XWindow::showEvent(QShowEvent* event) {
    taskbar_->showEvent();
}

void XWindow::paintEvent(QPaintEvent* event) {
    /*QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QRectF rect(10, 10, this->width() - 20, this->height() - 20);
    path.addRoundRect(rect, 8, 8);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(Qt::white));

    QColor color(0, 0, 0, 50);
    for (int i = 0; i < 10; i++) {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRect(10 - i, 10 - i, this->width() - (10 - i) * 2, this->height() - (10 - i) * 2);
        color.setAlpha(150 - qSqrt(i) * 50);
        painter.setPen(color);
        painter.drawPath(path);
    }*/
    return QWidget::paintEvent(event);
}
#endif
