#include <QApplication>
#include <QLayout>
#include <QStyle>
#include <QFontDatabase>
#include <QTranslator>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <windowsx.h>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>
#pragma comment(lib, "dwmapi.lib")
#include <dwmapi.h>
#include <widget/win32/fluentstyle.h>
#endif

#include "thememanager.h"
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/framelesswindow.h>

FramelessWindow::FramelessWindow(QWidget* parent)
    : QWidget(parent)
#if defined(Q_OS_WIN)
	, is_maximized_(false)
    , border_width_(5)
#endif
{    
    setAcceptDrops(true);
    setMouseTracking(true);
    installEventFilter(this);
#if defined(Q_OS_WIN)
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMaximizeButtonHint);
    BOOL is_dwm_enable = false;
    ::DwmIsCompositionEnabled(&is_dwm_enable);
    if (is_dwm_enable) {
        HWND hwnd = (HWND)winId();        
        DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
        ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
        MARGINS borderless = { -1 };
        ::DwmExtendFrameIntoClientArea(hwnd, &borderless);
    }
    ThemeManager::instance().enableBlur(this, AppSettings::getValueAsBool(kAppSettingEnableBlur));
    setupThumbnailToolBar();
    setStyleSheet(Q_UTF8(R"(
		background: transparent;        
        border: none;        
    )"));
#endif    
}

FramelessWindow::~FramelessWindow() {
}

void FramelessWindow::setupThumbnailToolBar() {
#if defined(Q_OS_WIN)
    play_icon_ = style()->standardIcon(QStyle::SP_MediaPlay);
    pause_icon_ = style()->standardIcon(QStyle::SP_MediaPause);
    stop_play_icon_ = style()->standardIcon(QStyle::SP_MediaStop);
    seek_forward_icon_ = style()->standardIcon(QStyle::SP_MediaSeekForward);
    seek_backward_icon_ = style()->standardIcon(QStyle::SP_MediaSeekBackward);
   
    thumbnail_tool_bar_.reset(new QWinThumbnailToolBar(this));
    thumbnail_tool_bar_->setWindow(windowHandle());

    taskbar_button_.reset(new QWinTaskbarButton(this));
    taskbar_button_->setWindow(windowHandle());
    taskbar_progress_ = taskbar_button_->progress();
    taskbar_progress_->setVisible(true);

    auto play_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
    play_tool_button->setIcon(play_icon_);
    (void)QObject::connect(play_tool_button,
        &QWinThumbnailToolButton::clicked,
        this,
        &FramelessWindow::play);

    auto forward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
    forward_tool_button->setIcon(seek_forward_icon_);
    (void)QObject::connect(forward_tool_button,
        &QWinThumbnailToolButton::clicked,
        this,
        &FramelessWindow::playNextClicked);

    auto backward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
    backward_tool_button->setIcon(seek_backward_icon_);
    (void)QObject::connect(backward_tool_button,
        &QWinThumbnailToolButton::clicked,
        this,
        &FramelessWindow::playPreviousClicked);

    thumbnail_tool_bar_->addButton(backward_tool_button);
    thumbnail_tool_bar_->addButton(play_tool_button);
    thumbnail_tool_bar_->addButton(forward_tool_button);
#endif
}

void FramelessWindow::setTaskbarProgress(const double percent) {
#if defined(Q_OS_WIN)
    taskbar_progress_->setValue(percent);
#else
    (void)percent;
#endif
}

void FramelessWindow::resetTaskbarProgress() {
#if defined(Q_OS_WIN)
    taskbar_progress_->reset();
    taskbar_progress_->setValue(0);
    taskbar_progress_->setRange(0, 100);
    taskbar_button_->setOverlayIcon(play_icon_);
    taskbar_progress_->show();
#endif
}

void FramelessWindow::setTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
    taskbar_button_->setOverlayIcon(play_icon_);
    taskbar_progress_->resume();
#endif
}

void FramelessWindow::setTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
    taskbar_button_->setOverlayIcon(pause_icon_);
    taskbar_progress_->pause();
#endif
}

void FramelessWindow::setTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
    resetTaskbarProgress();
#endif
}

void FramelessWindow::setTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
    taskbar_button_->setOverlayIcon(stop_play_icon_);
    taskbar_progress_->hide();
#endif
}

bool FramelessWindow::eventFilter(QObject * object, QEvent * event) {
    if (event->type() == QEvent::KeyPress) {
        const auto key_event = static_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            deleteKeyPress();
        }
    }
    return QWidget::eventFilter(object, event);
}

void FramelessWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void FramelessWindow::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void FramelessWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void FramelessWindow::dropEvent(QDropEvent* event) {
    const auto* mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        for (auto const& url : mime_data->urls()) {
            addDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

#if defined(Q_OS_WIN)
bool FramelessWindow::hitTest(MSG const* msg, long* result) const {
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

bool FramelessWindow::nativeEvent(const QByteArray& event_type, void * message, long * result) {
    #if defined(Q_OS_WIN)
    const auto msg = static_cast<MSG const*>(message);
    switch (msg->message) {
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = HTCAPTION;
            return hitTest(msg, result);
        }
        break;
    case WM_NCCALCSIZE:
        *result = 0;
        return true;
    default:
        break;
    }
    #endif
    return QWidget::nativeEvent(event_type, message, result);
}


void FramelessWindow::mousePressEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    mouse_pressed_pt_ = event->globalPos();
    win_drag_pos_pt_ = pos();
#else
    QWidget::mousePressEvent(event);
#endif
}

void FramelessWindow::mouseReleaseEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    mouse_pressed_pt_ = QPoint();
#else
    QWidget::mouseReleaseEvent(event);
#endif
}

void FramelessWindow::mouseMoveEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    if (!mouse_pressed_pt_.isNull()) {
        QPoint delta = event->globalPos() - mouse_pressed_pt_;
        move(win_drag_pos_pt_ + delta);
    }
#endif
    QWidget::mouseMoveEvent(event);
}

void FramelessWindow::showEvent(QShowEvent* event) {
    event->accept();
}

void FramelessWindow::addDropFileItem(const QUrl&) {
}

void FramelessWindow::deleteKeyPress() {
}
