#include <QTimer>
#include <QPainter>

#include <widget/win32/wintaskbar.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>

#if defined(Q_OS_WIN)
#include <widget/xmainwindow.h>
#include <widget/util/image_util.h>
#include <widget/widget_shared.h>

#include <base/dll.h>
#include <base/com_error_category.h>

#include <xampplayer.h>

#include <QWindow>
#include <QCoreApplication>

#include <dwmapi.h>

#include <Windows.h>
#include <shobjidl.h>

enum HBitmapFormat {
	HBitmapNoAlpha,
	HBitmapPremultipliedAlpha,
	HBitmapAlpha
};

enum {
	ToolButton_Backward = 0,
	ToolButton_Play,
	ToolButton_Forward,
	ToolButton_Pause,
};

Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap& p, int hbitmapFormat = HBitmapFormat::HBitmapNoAlpha);

namespace {
	typedef enum _WINDOWCOMPOSITIONATTRIB
	{
		WCA_UNDEFINED = 0,
		WCA_NCRENDERING_ENABLED = 1,
		WCA_NCRENDERING_POLICY = 2,
		WCA_TRANSITIONS_FORCEDISABLED = 3,
		WCA_ALLOW_NCPAINT = 4,
		WCA_CAPTION_BUTTON_BOUNDS = 5,
		WCA_NONCLIENT_RTL_LAYOUT = 6,
		WCA_FORCE_ICONIC_REPRESENTATION = 7,
		WCA_EXTENDED_FRAME_BOUNDS = 8,
		WCA_HAS_ICONIC_BITMAP = 9,
		WCA_THEME_ATTRIBUTES = 10,
		WCA_NCRENDERING_EXILED = 11,
		WCA_NCADORNMENTINFO = 12,
		WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
		WCA_VIDEO_OVERLAY_ACTIVE = 14,
		WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
		WCA_DISALLOW_PEEK = 16,
		WCA_CLOAK = 17,
		WCA_CLOAKED = 18,
		WCA_ACCENT_POLICY = 19,
		WCA_FREEZE_REPRESENTATION = 20,
		WCA_EVER_UNCLOAKED = 21,
		WCA_VISUAL_OWNER = 22,
		WCA_HOLOGRAPHIC = 23,
		WCA_EXCLUDED_FROM_DDA = 24,
		WCA_PASSIVEUPDATEMODE = 25,
		WCA_USEDARKMODECOLORS = 26,
		WCA_CORNER_STYLE = 27,
		WCA_PART_COLOR = 28,
		WCA_DISABLE_MOVESIZE_FEEDBACK = 29,
		WCA_LAST = 30
	} WINDOWCOMPOSITIONATTRIB;

	struct GdiDeleter final {
		static HBITMAP invalid() noexcept {
			return nullptr;
		}

		static void close(HBITMAP value) {
			::DeleteObject(value);
		}
	};

	using GdiHandle = UniqueHandle<HBITMAP, GdiDeleter>;

	struct HIconDeleter final {
		static HICON invalid() noexcept {
			return nullptr;
		}

		static void close(HICON value) {
			::DestroyIcon(value);
		}
	};

	using HIconHandle = UniqueHandle<HICON, HIconDeleter>;

	class DwmapiLib {
	public:
		DwmapiLib()
			: module_(OpenSharedLibrary("dwmapi"))
			, XAMP_LOAD_DLL_API(DwmInvalidateIconicBitmaps)
			, XAMP_LOAD_DLL_API(DwmSetWindowAttribute)
			, XAMP_LOAD_DLL_API(DwmSetIconicThumbnail)
			, XAMP_LOAD_DLL_API(DwmSetIconicLivePreviewBitmap)
			, XAMP_LOAD_DLL_API(DwmExtendFrameIntoClientArea) {
		}

		XAMP_DISABLE_COPY(DwmapiLib)

	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(DwmInvalidateIconicBitmaps);
		XAMP_DECLARE_DLL_NAME(DwmSetWindowAttribute);
		XAMP_DECLARE_DLL_NAME(DwmSetIconicThumbnail);
		XAMP_DECLARE_DLL_NAME(DwmSetIconicLivePreviewBitmap);
		XAMP_DECLARE_DLL_NAME(DwmExtendFrameIntoClientArea);
	};

	TBPFLAG getWin32ProgressState(TaskbarProgressState state) {
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

#define DWM_DLL Singleton<DwmapiLib>::GetInstance()
#define IDTB_FIRST 3000

	UINT MSG_TaskbarButtonCreated = WM_NULL;

	void updateLiveThumbnail(HWND hwnd, const QPixmap& thumbnail) {
		RECT rect{};
		if (!::GetClientRect(hwnd, &rect)) {
			XAMP_LOG_ERROR("GetClientRect return failure! {}", GetLastErrorMessage());
			return;
		}

		const QSize max_size(rect.right - rect.left, rect.bottom - rect.top);
		POINT offset = { 0, 0 };

		QPixmap resize_image;
		if (max_size != thumbnail.size()) {
			resize_image = image_util::resizeImage(thumbnail, max_size, false);
			Q_ASSERT(resize_image.size() == max_size);
		}
		else {
			resize_image = thumbnail;
		}

		const GdiHandle bitmap(qt_pixmapToWinHBITMAP(resize_image));
		if (!bitmap) {
			XAMP_LOG_ERROR("Failure to convert QPixmap to HBITMAP! ({})", GetLastErrorMessage());
			return;
		}

		const auto hr = DWM_DLL.DwmSetIconicLivePreviewBitmap(hwnd, bitmap.get(), &offset, 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicLivePreviewBitmap return failure! ({})", com_to_system_error(hr).code().message());
		}		
	}

	void updateIconicThumbnail(const QSize &aero_peak_size, HWND hwnd, const QPixmap& thumbnail) {
		const auto src_width = static_cast<float>(thumbnail.width());
		const auto src_height = static_cast<float>(thumbnail.height());
		auto width_ratio = static_cast<float>(aero_peak_size.width()) / src_width;
		auto height_ratio = static_cast<float>(aero_peak_size.height()) / src_height;

		XAMP_LOG_DEBUG("thumbnail size: {}x{}", src_width, src_height);

		int thumbnail_width = src_width;
		int thumbnail_height = src_height;

		float scale = (std::min)(width_ratio, height_ratio);
		thumbnail_width = static_cast<int>(src_width * scale);
		thumbnail_height = static_cast<int>(src_height * scale);

		const QSize resize_size(thumbnail_width, thumbnail_height);

		QPixmap resize_image;
		if (resize_size != thumbnail.size()) {
			resize_image = image_util::resizeImage(thumbnail, resize_size, true);
		}
		else {
			resize_image = thumbnail;
		}

		XAMP_LOG_DEBUG("updateIconicThumbnail size: {}x{}", resize_image.width(), resize_image.height());

		const GdiHandle bitmap(qt_pixmapToWinHBITMAP(resize_image, HBitmapPremultipliedAlpha));
		if (!bitmap) {
			return;
		}

		const auto hr = DWM_DLL.DwmSetIconicThumbnail(hwnd, bitmap.get(), 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicThumbnail return failure! {}", GetPlatformErrorMessage(hr));
		}
	}

	constexpr UINT ID_BACKWARD   = 3001;
	constexpr UINT ID_PLAY_PAUSE = 3002;
	constexpr UINT ID_STOP       = 3003;
	constexpr UINT ID_FORWARD    = 3004;
}

struct WinTaskbar::ButtonIcon {
	int32_t getIconSize() const {
		return ::GetSystemMetrics(SM_CXSMICON);
	}

	HIconHandle play_icon;
	HIconHandle pause_icon;
	HIconHandle stop_play_icon;
	HIconHandle seek_forward_icon;
	HIconHandle seek_backward_icon;
};

WinTaskbar::WinTaskbar(XMainWindow* window, IXFrame* frame)
	: button_icons_(MakeAlign<ButtonIcon>()) {
	auto hr = ::CoCreateInstance(CLSID_TaskbarList,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList3,
		reinterpret_cast<void**>(&taskbar_list_));

	if (FAILED(hr)) {
		XAMP_LOG_ERROR("Failure to create IID_ITaskbarList3 ({}).", GetLastErrorMessage());
	}

	hr = taskbar_list_->HrInit();
	if (FAILED(hr)) {
		taskbar_list_.Release();
		XAMP_LOG_ERROR("Failure to create IID_ITaskbarList3 ({}).", GetLastErrorMessage());
		return;
	}

	if (MSG_TaskbarButtonCreated == WM_NULL) {
		MSG_TaskbarButtonCreated = ::RegisterWindowMessageW(L"TaskbarButtonCreated");
	}
	if (MSG_TaskbarButtonCreated == WM_NULL) {
		taskbar_list_.Release();
		XAMP_LOG_ERROR("Failure to RegisterWindowMessageW ({}).", GetLastErrorMessage());
		return;
	}

	const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
	setTheme(theme);
	setWindow(window);
	frame_ = frame;
	setRange(0, 100);
}

WinTaskbar::~WinTaskbar() = default;

void WinTaskbar::setTheme(ThemeColor theme_color) {
	play_icon = qTheme.fontIcon(Glyphs::ICON_PLAY, theme_color);
	pause_icon = qTheme.fontIcon(Glyphs::ICON_PAUSE, theme_color);
	stop_play_icon = qTheme.fontIcon(Glyphs::ICON_STOP_PLAY, theme_color);
	seek_forward_icon = qTheme.fontIcon(Glyphs::ICON_PLAY_FORWARD, theme_color);
	seek_backward_icon = qTheme.fontIcon(Glyphs::ICON_PLAY_BACKWARD, theme_color);
}

void WinTaskbar::setWindow(QWidget* window) {
	window_ = window;
	QCoreApplication::instance()->installNativeEventFilter(this);

	if (window_->isVisible()) {
		updateProgressIndicator();
		updateOverlay();
		addThumbnailButtons();
	}

	constexpr BOOL enable = TRUE;
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());

	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_HAS_ICONIC_BITMAP, &enable, sizeof(enable));
	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &enable, sizeof(enable));
	DWM_DLL.DwmInvalidateIconicBitmaps(hwnd);
}

void WinTaskbar::setRange(int progress_minimum, int progress_maximum) {
	const bool min_changed = progress_minimum != process_min_;
	const bool max_changed = progress_maximum != process_max_;

	if (!min_changed && !max_changed)
		return;

	process_min_ = progress_minimum;
	process_max_ = (std::max)(progress_minimum, progress_maximum);

	if (process_value_ < process_min_ || process_value_ > process_max_)
		resetTaskbarProgress();

	updateProgressIndicator();
}

void WinTaskbar::updateProgressIndicator() {
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
	hr = taskbar_list_->SetProgressState(hwnd, getWin32ProgressState(state_));

	if (FAILED(hr)) {
		XAMP_LOG_ERROR("UpdateProgressIndicator return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::setIconicThumbnail(const QPixmap& image) {
	if (!window_) {
		return;
	}

	XAMP_LOG_DEBUG("setIconicThumbnail width:{} height:{}", image.width(), image.height());

	thumbnail_ = image;
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	auto hr = DWM_DLL.DwmInvalidateIconicBitmaps(hwnd);
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("DwmInvalidateIconicBitmaps return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::updateOverlay() {
	if (!window_) {
		return;
	}

	if (!taskbar_list_) {
		return;
	}

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());

	std::wstring description;
	HIconHandle icon_handle;

	if (!overlay_accessible_description_.isEmpty())
		description = overlay_accessible_description_.toStdWString();

	if (!overlay_icon_.isNull()) {
		icon_handle.reset(overlay_icon_.pixmap(button_icons_->getIconSize()).toImage().toHICON());

		if (!icon_handle) {
			icon_handle.reset(static_cast<HICON>(::LoadImage(nullptr,
				IDI_APPLICATION,
				IMAGE_ICON,
				SM_CXSMICON, 
				SM_CYSMICON, 
				LR_SHARED)));
		}
	}

	if (icon_handle) {
		auto hr = taskbar_list_->SetOverlayIcon(hwnd, icon_handle.get(), description.c_str());
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("UpdateOverlay return failure! {}", GetPlatformErrorMessage(hr));
		}
	}
}

void WinTaskbar::addThumbnailButtons() {
	if (!taskbar_list_ || !window_) {
		return;
	}

	HWND hwnd = reinterpret_cast<HWND>(window_->winId());
	button_icons_->seek_backward_icon.reset(seek_backward_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());
	button_icons_->play_icon.reset(play_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());
	button_icons_->pause_icon.reset(pause_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());
	button_icons_->stop_play_icon.reset(stop_play_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());
	button_icons_->seek_forward_icon.reset(seek_forward_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());

	std::array<THUMBBUTTON, 3> buttons{};

	auto init_thumb_button = [&](THUMBBUTTON& btn,
		UINT iId,
		HICON hIcon,
		LPCWSTR tip) {
			btn.dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
			btn.dwFlags = THBF_ENABLED;
			btn.iId = iId;
			btn.hIcon = hIcon;
			wcsncpy_s(btn.szTip, tip, _TRUNCATE);
		};

	// Default play icon
	auto play_icon = button_icons_->play_icon.get();

	init_thumb_button(buttons[0], ID_BACKWARD,button_icons_->seek_backward_icon.get(), L"Backward");
	init_thumb_button(buttons[1], ID_PLAY_PAUSE, play_icon, L"Play");
	init_thumb_button(buttons[2], ID_FORWARD, button_icons_->seek_forward_icon.get(), L"Forward");

	HRESULT hr = taskbar_list_->ThumbBarAddButtons(hwnd, buttons.size(),
	                                               buttons.data());
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("ThumbBarAddButtons failed: {}", GetPlatformErrorMessage(hr));
	}
}

bool WinTaskbar::nativeEventFilter(const QByteArray& event_type, void* message, qintptr* result) {
	const auto* msg = static_cast<MSG*>(message);
	if (msg->message == MSG_TaskbarButtonCreated) {
		updateProgressIndicator();
		updateOverlay();
		return true;
	}

	switch (msg->message) {
	case WM_COMMAND: {
		const auto id = LOWORD(msg->wParam);
		switch (id) {
		case ID_BACKWARD:
			frame_->playPrevious();
			break;
		case ID_PLAY_PAUSE:
			frame_->playOrPause();
			break;
		case ID_STOP:
			frame_->stopPlay();
			break;
		case ID_FORWARD:
			frame_->playNext();
			break;
		default:
			break;
		}
		if (result != nullptr) {
			*result = 0;
		}
		return true;
	}
	case WM_DWMSENDICONICTHUMBNAIL: {
		const int requested_width = LOWORD(msg->lParam);
		const int requested_height = HIWORD(msg->lParam);
		if (requested_width <= 1 || requested_height <= 1) {
			XAMP_LOG_WARN("WM_DWMSENDICONICTHUMBNAIL width:{} height:{}", requested_width, requested_height);
			return false;
		}
		QSize target_size(requested_width, requested_height);
		updateIconicThumbnail(target_size, msg->hwnd, thumbnail_);
		if (result != nullptr) {
			*result = 0;
		}
		return true;
	}
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP: 
		XAMP_LOG_DEBUG("WM_DWMSENDICONICLIVEPREVIEWBITMAP");
		updateLiveThumbnail(msg->hwnd, window_->grab());
		if (result != nullptr) {
			*result = 0;
		}
		return true;
	default:
		break;
	}
	return false;
}

void WinTaskbar::updateThumbnailButton(UINT iId, HICON hIcon, LPCWSTR tooltip) {
	if (!taskbar_list_ || !window_) {
		return;
	}

	THUMBBUTTON btn{};
	btn.dwMask = THB_FLAGS | THB_ICON | THB_TOOLTIP;
	btn.dwFlags = THBF_ENABLED;
	btn.iId = iId;
	btn.hIcon = hIcon;
	wcsncpy_s(btn.szTip, tooltip, _TRUNCATE);

	HWND hwnd = reinterpret_cast<HWND>(window_->winId());
	HRESULT hr = taskbar_list_->ThumbBarUpdateButtons(hwnd, 1, &btn);
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("ThumbBarUpdateButtons failed: {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::setTaskbarProgress(const int32_t process) {
	if (process == process_value_ || process < process_min_ || process > process_max_)
		return;

	if (state_ == TASKBAR_PROCESS_STATE_INDETERMINATE)
		state_ = TASKBAR_PROCESS_STATE_NORMAL;

	process_value_ = process;
	updateProgressIndicator();
}

void WinTaskbar::resetTaskbarProgress() {
	setRange(0, 100);
	state_ = TASKBAR_PROCESS_STATE_NO_PROCESS;
	setTaskbarProgress(0);
	setTaskbarPlayingResume();
}

void WinTaskbar::setTaskbarPlayingResume() {
	overlay_icon_ = play_icon;
	state_ = TASKBAR_PROCESS_STATE_NORMAL;
	updateProgressIndicator();
	updateOverlay();
	button_icons_->pause_icon.reset(pause_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());
	updateThumbnailButton(ID_PLAY_PAUSE, button_icons_->pause_icon.get(), L"Pause");
}

void WinTaskbar::setTaskbarPlayerPaused() {
	overlay_icon_ = pause_icon;
	state_ = TASKBAR_PROCESS_STATE_PAUSED;
	updateProgressIndicator();
	updateOverlay();
	button_icons_->play_icon.reset(play_icon.pixmap(button_icons_->getIconSize()).toImage().toHICON());
	updateThumbnailButton(ID_PLAY_PAUSE, button_icons_->play_icon.get(), L"Play");
}

void WinTaskbar::setTaskbarPlayerPlaying() {
	resetTaskbarProgress();
}

void WinTaskbar::setTaskbarPlayerStop() {
	overlay_icon_ = stop_play_icon;
	state_ = TASKBAR_PROCESS_STATE_NO_PROCESS;
	updateProgressIndicator();
	updateOverlay();
}
#endif