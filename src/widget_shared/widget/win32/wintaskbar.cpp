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
	struct GdiDeleter final {
		static HBITMAP invalid() {
			return nullptr;
		}

		static void Close(HBITMAP value) {
			::DeleteObject(value);
		}
	};

	using GdiHandle = UniqueHandle<HBITMAP, GdiDeleter>;

	struct HIconDeleter final {
		static HICON invalid() {
			return nullptr;
		}

		static void Close(HICON value) {
			::DestroyIcon(value);
		}
	};

	using HIconHandle = UniqueHandle<HICON, HIconDeleter>;

	class DwmapiLib {
	public:
		XAMP_DECLARE_SINGLETON_NAME()

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
#define DwmDll SharedSingleton<DwmapiLib>::GetInstance()

	TBPFLAG convertToProgressState(TaskbarProgressState state) {
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

	GdiHandle createDwmCompatibleHBitmap(const QImage& srcPremulArgb32) {
		QImage img = srcPremulArgb32;
		if (img.format() != QImage::Format_ARGB32_Premultiplied) {
			img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		}

		BITMAPINFO bmi{};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = img.width();
		bmi.bmiHeader.biHeight = -img.height(); // top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void* bits = nullptr;
		HBITMAP hbmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
		if (!hbmp || !bits) return GdiHandle();

		// QImage::Format_ARGB32_Premultiplied 在 little-endian 下就是 BGRA 記憶體排列（DWM 可吃）
		const int bytes = img.bytesPerLine() * img.height();
		memcpy(bits, img.bits(), bytes);
		return GdiHandle(hbmp);
	}

	bool updateLiveThumbnail(HWND hwnd, const QPixmap& thumbnail) {
		RECT rect{};
		if (!::GetClientRect(hwnd, &rect)) {
			XAMP_LOG_ERROR("GetClientRect return failure! {}", GetLastErrorMessage());
			return false;
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

		const GdiHandle bitmap = createDwmCompatibleHBitmap(resize_image.toImage());
		if (!bitmap) {
			XAMP_LOG_ERROR("Failure to convert QPixmap to HBITMAP! ({})", GetLastErrorMessage());
			return false;
		}

		const auto hr = DwmDll.DwmSetIconicLivePreviewBitmap(hwnd, bitmap.get(), &offset, 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicLivePreviewBitmap return failure! ({})",
				GetPlatformErrorMessage(hr));
			return false;
		}		
		return true;
	}

	bool updateIconicThumbnail(const QSize &aero_peak_size, HWND hwnd, const QPixmap& thumbnail) {
		if (thumbnail.isNull() || thumbnail.width() <= 0 || thumbnail.height() <= 0) {
			return false;
		}

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

		XAMP_LOG_DEBUG("scale: {}", scale);

		const QSize resize_size(thumbnail_width, thumbnail_height);

		QPixmap resize_image;
		if (resize_size != thumbnail.size()) {
			resize_image = image_util::resizeImage(thumbnail, resize_size, true);
		}
		else {
			resize_image = thumbnail;
		}

		XAMP_LOG_DEBUG("updateIconicThumbnail size: {}x{}", resize_image.width(), resize_image.height());
		
		XAMP_LOG_DEBUG("updateIconicThumbnail size: {}x{} aero_peak_size: {}x{}", 
			resize_image.width(), resize_image.height(), aero_peak_size.width(), aero_peak_size.height());

		const GdiHandle bitmap(createDwmCompatibleHBitmap(resize_image.toImage()));
		if (!bitmap) {
			return false;
		}

		const auto hr = DwmDll.DwmSetIconicThumbnail(hwnd, bitmap.get(), 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicThumbnail return failure! {}",
				GetPlatformErrorMessage(hr));
			return false;
		}
		return true;
	}

	constexpr UINT IDTB_FIRST     = 3000;
	UINT MSG_TaskbarButtonCreated = WM_NULL;
	constexpr UINT ID_BACKWARD    = 3001;
	constexpr UINT ID_PLAY_PAUSE  = 3002;
	constexpr UINT ID_STOP        = 3003;
	constexpr UINT ID_FORWARD     = 3004;
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
	frame_ = frame;

	auto hr = ::CoCreateInstance(CLSID_TaskbarList,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList4,
		reinterpret_cast<void**>(&taskbar_list_));

	if (FAILED(hr)) {
		XAMP_LOG_ERROR("Failure to create IID_ITaskbarList4 ({}).", GetPlatformErrorMessage(hr));
		return;
	}

	hr = taskbar_list_->HrInit();
	if (FAILED(hr)) {
		taskbar_list_.Release();
		XAMP_LOG_ERROR("Failure to init ITaskbarList4 ({}).", GetPlatformErrorMessage(hr));
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
	setRange(0, 100);
}

WinTaskbar::~WinTaskbar() {
	if (auto* app = QCoreApplication::instance()) {
		app->removeNativeEventFilter(this);
	}
}

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

	DwmDll.DwmSetWindowAttribute(hwnd, DWMWA_HAS_ICONIC_BITMAP, &enable, sizeof(enable));
	DwmDll.DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &enable, sizeof(enable));
	DwmDll.DwmInvalidateIconicBitmaps(hwnd);
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
	hr = taskbar_list_->SetProgressState(hwnd, convertToProgressState(state_));

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
	auto hr = DwmDll.DwmInvalidateIconicBitmaps(hwnd);
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("DwmInvalidateIconicBitmaps return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::updateOverlay() {
	XAMP_LOG_DEBUG("updateOverlay");

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
	XAMP_LOG_DEBUG("addThumbnailButtons");

	if (!taskbar_list_ || !window_) {
		return;
	}

	if (thumbnail_buttons_added_) {
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
		return;
	}
	thumbnail_buttons_added_ = true;
}

bool WinTaskbar::nativeEventFilter(const QByteArray& event_type, void* message, qintptr* result) {
	(void)event_type;

	if (!window_) {
		return false;
	}

	const auto* msg = static_cast<MSG*>(message);
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	if (msg->hwnd != hwnd) {
		return false;
	}

	if (msg->message == MSG_TaskbarButtonCreated) {
		updateProgressIndicator();
		updateOverlay();
		addThumbnailButtons();
		if (result != nullptr) {
			*result = 0;
		}
		return true;
	}

	switch (msg->message) {
	case WM_COMMAND: {
		const auto id = LOWORD(msg->wParam);
		switch (id) {
		case ID_BACKWARD:
			if (!frame_) {
				return false;
			}
			frame_->playPrevious();
			break;
		case ID_PLAY_PAUSE:
			if (!frame_) {
				return false;
			}
			frame_->playOrPause();
			break;
		case ID_STOP:
			if (!frame_) {
				return false;
			}
			frame_->stopPlay();
			break;
		case ID_FORWARD:
			if (!frame_) {
				return false;
			}
			frame_->playNext();
			break;
		default:
			return false;
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
		if (!updateIconicThumbnail(target_size, msg->hwnd, thumbnail_)) {
			// If the thumbnail is not set or failed to update, use a default unknown cover image
			updateIconicThumbnail(target_size, msg->hwnd, qTheme.unknownCover());
		}
		if (result != nullptr) {
			*result = 0;
		}
		return true;
	}
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP: {
		const auto live_image = window_->grab();
		XAMP_LOG_DEBUG("WM_DWMSENDICONICLIVEPREVIEWBITMAP {}:{}", live_image.width(), live_image.height());
		updateLiveThumbnail(msg->hwnd, live_image);
		if (result != nullptr) {
			*result = 0;
		}
		return true;
		}
	default:
		break;
	}
	return false;
}

void WinTaskbar::updateThumbnailButton(UINT iId, HICON hIcon, LPCWSTR tooltip) {
	XAMP_LOG_DEBUG("updateThumbnailButton");

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
