#include <widget/win32/win32.h>
#include <widget/xmainwindow.h>
#include <xampplayer.h>

#include <Windows.h>

#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>

namespace win32 {

WinTaskbar::WinTaskbar(XMainWindow* window, IXFrame* content_widget) {
	window_ = window;

	play_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_LIST_PLAY);
	pause_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_LIST_PAUSE);
	stop_play_icon = qTheme.GetFontIcon(Glyphs::ICON_STOP_PLAY);
	seek_forward_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_FORWARD);
	seek_backward_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_BACKWARD);

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
		&IXFrame::PlayOrPause);

	auto* forward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
	forward_tool_button->setIcon(seek_forward_icon);
	(void)QObject::connect(forward_tool_button,
		&QWinThumbnailToolButton::clicked,
		content_widget,
		&IXFrame::PlayNext);

	auto* backward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
	backward_tool_button->setIcon(seek_backward_icon);
	(void)QObject::connect(backward_tool_button,
		&QWinThumbnailToolButton::clicked,
		content_widget,
		&IXFrame::PlayPrevious);

	thumbnail_tool_bar_->addButton(backward_tool_button);
	thumbnail_tool_bar_->addButton(play_tool_button);
	thumbnail_tool_bar_->addButton(forward_tool_button);
}

WinTaskbar::~WinTaskbar() = default;

void WinTaskbar::SetTaskbarProgress(const int32_t percent) {
	taskbar_progress_->setValue(percent);
}

void WinTaskbar::ResetTaskbarProgress() {
	taskbar_progress_->reset();
	taskbar_progress_->setValue(0);
	taskbar_progress_->setRange(0, 100);
	taskbar_button_->setOverlayIcon(play_icon);
	taskbar_progress_->show();
}

void WinTaskbar::SetTaskbarPlayingResume() {
	taskbar_button_->setOverlayIcon(play_icon);
	taskbar_progress_->resume();
}

void WinTaskbar::SetTaskbarPlayerPaused() {
	taskbar_button_->setOverlayIcon(pause_icon);
	taskbar_progress_->pause();
}

void WinTaskbar::SetTaskbarPlayerPlaying() {
	ResetTaskbarProgress();
}

void WinTaskbar::SetTaskbarPlayerStop() {
	taskbar_button_->setOverlayIcon(stop_play_icon);
	taskbar_progress_->hide();
}

void WinTaskbar::showEvent() {
	taskbar_button_->setWindow(window_->windowHandle());
}

QRect GetWindowRect(const WId window_id) {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	RECT rect{ 0 };
	::GetWindowRect(hwnd, &rect);
	auto width = rect.right - rect.left;
	auto height = rect.bottom - rect.top;
	return QRect(rect.left, rect.top, width, height);
}

}

