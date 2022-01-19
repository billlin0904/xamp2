#include <QApplication>
#include <QLayout>
#include <QFontDatabase>
#include <QPainter>
#include <QWindow>
#include <QStyleOption>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QIcon>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <windowsx.h>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>
#include <widget/win32/win32.h>
#else
#include <widget/osx/osx.h>
#endif

#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/xwindow.h>

#include "thememanager.h"

struct XWindow::WinTaskbar {
#if defined(Q_OS_WIN)
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
    QScopedPointer<QWinThumbnailToolBar> thumbnail_tool_bar_;
    QScopedPointer<QWinTaskbarButton> taskbar_button_;
    QWinTaskbarProgress* taskbar_progress_;
    XWindow* window_;
#endif
};

XWindow::XWindow()
    : IXWindow()
#if defined(Q_OS_WIN)
    , border_width_(5)
    , current_screen_(nullptr)
#endif
	, content_widget_(nullptr) {
    setObjectName(Q_UTF8("framelessWindow"));
}

void XWindow::setContentWidget(IXampPlayer *content_widget) {
    // todo: DropShadow效果會讓CPU使用率偏高.
    // todo: 使用border-radius: 5px;將無法縮放視窗
    // todo: Qt::WA_TranslucentBackground + paintEvent 將無法縮放視窗
#if defined(Q_OS_WIN)
    if (!ThemeManager::instance().useNativeWindow()) {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        //setAttribute(Qt::WA_TranslucentBackground);
    }
#endif
    content_widget_ = content_widget;    
    if (content_widget_ != nullptr) {        
        auto* default_layout = new QGridLayout();
        default_layout->addWidget(content_widget_, 0, 0);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }
    setAcceptDrops(true);
    setMouseTracking(true);
    installEventFilter(this);
#if defined(Q_OS_WIN)
    if (!ThemeManager::instance().useNativeWindow()) {
        win32::setFramelessWindowStyle(this);
        win32::drawDwmShadow(this);
        setWindowTitle(kAppTitle);
    }
    taskbar_.reset(new WinTaskbar(this, content_widget));
#else
    if (!ThemeManager::instance().useNativeWindow()) {
        osx::hideTitleBar(content_widget_);
        setWindowTitle(kAppTitle);
    }
#endif
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

#if defined(Q_OS_WIN)
bool XWindow::hitTest(MSG const* msg, long* result) const {
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

bool XWindow::nativeEvent(const QByteArray& event_type, void * message, long * result) {
    if (ThemeManager::instance().useNativeWindow()) {
        return QWidget::nativeEvent(event_type, message, result);
    }

#if defined(Q_OS_WIN)
    const auto *msg = static_cast<MSG const*>(message);
    switch (msg->message) {
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = HTCAPTION;
            return hitTest(msg, result);
        }
        break;
    case WM_GETMINMAXINFO:
        if (layout() != nullptr) {
            if (::IsZoomed(msg->hwnd)) {
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
            *result = 0;
            return true;
        }
        /*
        if (!::IsZoomed(msg->hwnd)) {
            *result = WVR_REDRAW;
            return true;
        } else {
            *result = 0;
            return true;
        }
        */
        *result = WVR_REDRAW;
        return true;
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
#endif
}

void XWindow::changeEvent(QEvent* event) {
#if defined(Q_OS_MAC)
    if (!ThemeManager::instance().useNativeWindow() && content_widget_ != nullptr) {
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

void XWindow::mouseReleaseEvent(QMouseEvent* event) {
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

void XWindow::mouseMoveEvent(QMouseEvent* event) {
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
#ifdef Q_OS_WIN32
void XWindow::showEvent(QShowEvent* event) {
    taskbar_->showEvent();
}
#endif
