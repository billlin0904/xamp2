#include <widget/win32/wintaskbar.h>
#include <widget/xmainwindow.h>
#include <xampplayer.h>

#include <QWindow>

#include <Windows.h>
#include <shobjidl.h>

namespace win32 {

static TBPFLAG GetWin32ProgressState(TaskbarProgressState state) {
	static const QMap<TaskbarProgressState, TBPFLAG> state_lut{
		{ TASKBAR_PROCESS_STATE_NO_PROCESS, TBPF_NOPROGRESS },
		{ TASKBAR_PROCESS_STATE_INDETERMINATE, TBPF_INDETERMINATE },
		{ TASKBAR_PROCESS_STATE_NORMAL, TBPF_NORMAL },
		{ TASKBAR_PROCESS_STATE_ERROR, TBPF_ERROR },
		{ TASKBAR_PROCESS_STATE_PAUSED, TBPF_PAUSED },
	};
	if (state_lut.contains(state)) {
		return state_lut.value(state);
	}
	return TBPF_NOPROGRESS;
}

static int GetWin32IconSize() {
	return ::GetSystemMetrics(SM_CXSMICON);
}

WinTaskbar::WinTaskbar(XMainWindow* window, IXFrame* content_widget) {
	play_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_LIST_PLAY);
	pause_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_LIST_PAUSE);
	stop_play_icon = qTheme.GetFontIcon(Glyphs::ICON_STOP_PLAY);
	seek_forward_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_FORWARD);
	seek_backward_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_BACKWARD);

	::CoCreateInstance(CLSID_TaskbarList,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList4,
		reinterpret_cast<void**>(&taskbar_list_));

	SetWindow(window);
}

void WinTaskbar::SetWindow(QWidget* window) {
	window->removeEventFilter(this);
	window_ = window;

	window_->installEventFilter(this);
	if (window_->isVisible()) {
		UpdateProgressIndicator();
		UpdateOverlay();
	}
}

void WinTaskbar::SetRange(int progress_minimum, int progress_maximum) {
	const bool min_changed = progress_minimum != process_min_;
	const bool max_changed = progress_maximum != process_max_;

	if (!min_changed && !max_changed)
		return;

	process_min_ = progress_minimum;
	process_max_ = std::max(progress_minimum, progress_maximum);

	if (process_value_ < process_min_ || process_value_ > process_max_)
		ResetTaskbarProgress();

	UpdateProgressIndicator();
}

void WinTaskbar::UpdateProgressIndicator() {
	if (!window_) {
		return;
	}

	if (!taskbar_list_) {
		return;
	}

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	HRESULT hr = S_OK;

	const auto progress_range = process_max_ - process_min_;
	if (progress_range > 0) {
		const int scaled_value = std::round(static_cast<double>(100) 
			* static_cast<double>(process_value_ - process_min_) / static_cast<double>(progress_range));
		hr = taskbar_list_->SetProgressValue(hwnd, static_cast<ULONGLONG>(scaled_value), 100);
	} else if (state_ == TASKBAR_PROCESS_STATE_NORMAL) {
		state_ = TASKBAR_PROCESS_STATE_INDETERMINATE;
	}
	hr = taskbar_list_->SetProgressState(hwnd, GetWin32ProgressState(state_));

	if (FAILED(hr)) {
		XAMP_LOG_DEBUG("UpdateProgressIndicator return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::UpdateOverlay() {
	if (!window_) {
		return;
	}

	if (!taskbar_list_) {
		return;
	}

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());

	std::wstring description;
	HICON icon_handle = nullptr;

	if (!overlay_accessible_description_.isEmpty())
		description = overlay_accessible_description_.toStdWString();

	if (!overlay_icon_.isNull()) {
		icon_handle = overlay_icon_.pixmap(GetWin32IconSize()).toImage().toHICON();

		if (!icon_handle)
			icon_handle = static_cast<HICON>(LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, SM_CXSMICON, SM_CYSMICON, LR_SHARED));
	}

	auto hr = taskbar_list_->SetOverlayIcon(hwnd, icon_handle, description.c_str());
	if (FAILED(hr)) {
		XAMP_LOG_DEBUG("UpdateOverlay return failure! {}", GetPlatformErrorMessage(hr));
	}

	if (icon_handle)
		::DestroyIcon(icon_handle);
}

bool WinTaskbar::eventFilter(QObject* object, QEvent* event) {
	if (object == window_ && event->type() == MSG_TaskbarButtonCreated) {
		UpdateProgressIndicator();
		UpdateOverlay();
	}
	return false;
}

WinTaskbar::~WinTaskbar() = default;

void WinTaskbar::SetTaskbarProgress(const int32_t process) {
	if (process == process_value_ || process < process_min_ || process > process_max_)
		return;

	if (state_ == TASKBAR_PROCESS_STATE_INDETERMINATE)
		state_ = TASKBAR_PROCESS_STATE_NORMAL;

	process_value_ = process;
	UpdateProgressIndicator();
}

void WinTaskbar::ResetTaskbarProgress() {
	SetRange(0, 100);
	state_ = TASKBAR_PROCESS_STATE_NO_PROCESS;
	SetTaskbarProgress(0);
	SetTaskbarPlayingResume();
}

void WinTaskbar::SetTaskbarPlayingResume() {
	overlay_icon_ = play_icon;
	state_ = TASKBAR_PROCESS_STATE_NORMAL;
	UpdateOverlay();
}

void WinTaskbar::SetTaskbarPlayerPaused() {
	overlay_icon_ = pause_icon;
	state_ = TASKBAR_PROCESS_STATE_PAUSED;
	UpdateProgressIndicator();
	UpdateOverlay();
}

void WinTaskbar::SetTaskbarPlayerPlaying() {
	ResetTaskbarProgress();
}

void WinTaskbar::SetTaskbarPlayerStop() {
	overlay_icon_ = stop_play_icon;
	state_ = TASKBAR_PROCESS_STATE_ERROR;
	UpdateProgressIndicator();
	UpdateOverlay();
}

}

