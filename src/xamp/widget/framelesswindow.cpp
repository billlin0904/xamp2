#include <QApplication>
#include <QLayout>
#include <QStyle>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <windowsx.h>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>
#pragma comment(lib, "dwmapi.lib")
#include <dwmapi.h>
#include <widget/win32/blur_effect_helper.h>
#endif

#include "widget/str_utilts.h"
#include "widget/framelesswindow.h"

FramelessWindow::FramelessWindow(QWidget* parent)
    : QWidget(parent)
	, is_maximized_(false)
	, border_width_(5) {	
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMaximizeButtonHint);
    setMouseTracking(true);
    installEventFilter(this);
    setAcceptDrops(true);
#if defined(Q_OS_WIN)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	HWND hwnd = (HWND)winId();
	const DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
	::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

	// 保留1個像素系統才會繪製陰影.
	const MARGINS borderless = { 1, 1, 1, 1 };
	::DwmExtendFrameIntoClientArea(hwnd, &borderless);

	setBlurMaterial(this);
#endif

	initialFontDatabase();
	setStyleSheet(Q_UTF8(R"(
		font-family: "UI";
		background: transparent;
	)"));
}

FramelessWindow::~FramelessWindow() {
}

void FramelessWindow::lazyInitial() {
#if defined(Q_OS_WIN)
	if (!taskbar_button_) {
		play_icon_ = style()->standardIcon(QStyle::SP_MediaPlay);
		pause_icon_ = style()->standardIcon(QStyle::SP_MediaPause);
		stop_play_icon_ = style()->standardIcon(QStyle::SP_MediaStop);
		seek_forward_icon_ = style()->standardIcon(QStyle::SP_MediaSeekForward);
		seek_backward_icon_ = style()->standardIcon(QStyle::SP_MediaSeekBackward);

		taskbar_button_ = xamp::base::MakeAlign<QWinTaskbarButton>();
		taskbar_button_->setWindow(windowHandle());
		taskbar_progress_ = taskbar_button_->progress();
		taskbar_progress_->setVisible(true);

		thumbnail_tool_bar_ = xamp::base::MakeAlign<QWinThumbnailToolBar>();

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
	}
#endif
}

void FramelessWindow::initialFontDatabase() {
	const QStringList fallback_fonts{
		"SF Pro Text",
		"SF Pro Icons",
		"Helvetica Neue",
		"Microsoft Yahei",
		"Helvetica",
		"Arial",
	};

	QFont::insertSubstitutions("UI", fallback_fonts);

	QFont default_font;
	default_font.setFamily("UI");
	default_font.setStyleStrategy(QFont::PreferAntialias);
	setFont(default_font);
}

void FramelessWindow::setTaskbarProgress(const double percent) {
#if defined(Q_OS_WIN)
	lazyInitial();
    taskbar_progress_->setValue(percent);
#endif
}

void FramelessWindow::resetTaskbarProgress() {
#if defined(Q_OS_WIN)
	lazyInitial();
    taskbar_progress_->reset();
    taskbar_progress_->setValue(0);
    taskbar_progress_->setRange(0, 100);
    taskbar_button_->setOverlayIcon(play_icon_);
    taskbar_progress_->show();
#endif
}

void FramelessWindow::setTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
	lazyInitial();
    taskbar_button_->setOverlayIcon(play_icon_);
    taskbar_progress_->resume();
#endif
}

void FramelessWindow::setTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
	lazyInitial();
    taskbar_button_->setOverlayIcon(pause_icon_);
    taskbar_progress_->pause();
#endif
}

void FramelessWindow::setTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
	lazyInitial();
    resetTaskbarProgress();
#endif
}

void FramelessWindow::setTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
	lazyInitial();
    taskbar_button_->setOverlayIcon(stop_play_icon_);
    taskbar_progress_->hide();
#endif
}

bool FramelessWindow::eventFilter(QObject * object, QEvent * event) {
    if (event->type() == QEvent::KeyPress) {
        const auto key_event = static_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            onDeleteKeyPress();
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
    const auto * mime_data = event->mimeData();

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

bool FramelessWindow::nativeEvent(const QByteArray & event_type, void * message, long * result) {
    const auto msg = static_cast<MSG const*>(message);
    switch (msg->message) {
    case WM_NCHITTEST:
		*result = HTCAPTION;
        return hitTest(msg, result);
    case WM_NCCALCSIZE:
        *result = 0;
        return true;
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
}

void FramelessWindow::mousePressEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    if (::ReleaseCapture()) {
        const auto widget = window();
        if (widget->isTopLevel()) {
            ::SendMessage(HWND(widget->winId()), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
        }
    }
#endif
    event->ignore();
}

void FramelessWindow::mouseReleaseEvent(QMouseEvent* event) {
    return QWidget::mouseReleaseEvent(event);
}

void FramelessWindow::mouseMoveEvent(QMouseEvent* event) {
    return QWidget::mouseMoveEvent(event);
}

void FramelessWindow::showEvent(QShowEvent* event) {
    event->accept();
}

void FramelessWindow::addDropFileItem(const QUrl& url) {
}

void FramelessWindow::onDeleteKeyPress() {
}

void FramelessWindow::play() {
}

void FramelessWindow::playNextClicked() {
}

void FramelessWindow::playPreviousClicked() {
}

void FramelessWindow::stopPlayedClicked() {
}