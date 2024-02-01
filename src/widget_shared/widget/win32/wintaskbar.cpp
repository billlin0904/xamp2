#include <QTimer>
#include <widget/win32/wintaskbar.h>

#if defined(Q_OS_WIN)
#include <widget/xmainwindow.h>
#include <widget/util/image_utiltis.h>
#include <widget/widget_shared.h>
#include <base/dll.h>

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

	typedef struct _WINDOWCOMPOSITIONATTRIBDATA
	{
		WINDOWCOMPOSITIONATTRIB Attribute;
		PVOID Data;
		SIZE_T SizeOfData;
	} WINDOWCOMPOSITIONATTRIBDATA;

	typedef enum _ACCENT_STATE
	{
		ACCENT_DISABLED = 0,
		ACCENT_ENABLE_GRADIENT = 1,
		ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
		ACCENT_ENABLE_BLURBEHIND = 3,
		ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
		ACCENT_ENABLE_HOSTBACKDROP = 5,
		ACCENT_INVALID_STATE = 6
	} ACCENT_STATE;

	typedef enum DWM_SYSTEMBACKDROP_TYPE {
		DWMSBT_AUTO,
		DWMSBT_NONE,
		DWMSBT_MAINWINDOW,
		DWMSBT_TRANSIENTWINDOW,
		DWMSBT_TABBEDWINDOW
	} DWM_SYSTEMBACKDROP_TYPE;

	typedef struct _ACCENT_POLICY
	{
		ACCENT_STATE AccentState;
		DWORD AccentFlags;
		DWORD GradientColor;
		DWORD AnimationId;
	} ACCENT_POLICY;

	WINUSERAPI
	BOOL
	WINAPI
	SetWindowCompositionAttribute(
		_In_ HWND hWnd,
		_Inout_ WINDOWCOMPOSITIONATTRIBDATA* pAttrData);

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

	class Comctl32Lib {
	public:
		Comctl32Lib()
			: module_(OpenSharedLibrary("comctl32"))
			, XAMP_LOAD_DLL_API(ImageList_Create)
			, XAMP_LOAD_DLL_API(ImageList_Add)
			, XAMP_LOAD_DLL_API(ImageList_Destroy) {
		}

		XAMP_DISABLE_COPY(Comctl32Lib)

	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(ImageList_Create);
		XAMP_DECLARE_DLL_NAME(ImageList_Add);
		XAMP_DECLARE_DLL_NAME(ImageList_Destroy);
	};

#define COMCTL32_DLL Singleton<Comctl32Lib>::GetInstance()

	class User32Lib {
	public:
		User32Lib()
			: module_(OpenSharedLibrary("user32"))
			, XAMP_LOAD_DLL_API(SetWindowCompositionAttribute) {
		}

		XAMP_DISABLE_COPY(User32Lib)

	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(SetWindowCompositionAttribute);
	};

#define USER32_DLL Singleton<User32Lib>::GetInstance()

	struct HImageListDeleter final {
		static HIMAGELIST invalid() noexcept {
			return nullptr;
		}

		static void close(HIMAGELIST value) {
			COMCTL32_DLL.ImageList_Destroy(value);
		}
	};

	using HImageHandleListHandle = UniqueHandle<HIMAGELIST, HImageListDeleter>;

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

	int32_t getWin32IconSize() {
		return ::GetSystemMetrics(SM_CXSMICON);
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

		XAMP_LOG_DEBUG("{} {} => {} {}", max_size.width(), max_size.height(), thumbnail.width(), thumbnail.height());

		const GdiHandle bitmap(qt_pixmapToWinHBITMAP(image_utils::resizeImage(thumbnail, max_size, true)));
		if (!bitmap) {
			return;
		}

		const auto hr = DWM_DLL.DwmSetIconicLivePreviewBitmap(hwnd, bitmap.get(), &offset, 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicLivePreviewBitmap return failure! {}", GetPlatformErrorMessage(hr));
		}		
	}

	void updateIconicThumbnail(const QSize aero_peak_size, HWND hwnd, const QPixmap& thumbnail) {
		const auto src_width = thumbnail.width();
		const auto src_height = thumbnail.height();
		auto width_ratio = static_cast<float>(aero_peak_size.width()) / src_width;
		auto height_ratio = static_cast<float>(aero_peak_size.height()) / src_height;
		width_ratio = (std::min)(width_ratio, height_ratio);
		height_ratio = width_ratio;

		const QSize resize_size(width_ratio * src_width, height_ratio * src_height);
		const auto resize_thumbnail = image_utils::resizeImage(thumbnail, resize_size);

		const GdiHandle bitmap(qt_pixmapToWinHBITMAP(resize_thumbnail));
		if (!bitmap) {
			return;
		}

		const auto hr = DWM_DLL.DwmSetIconicThumbnail(hwnd, bitmap.get(), 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicThumbnail return failure! {}", GetPlatformErrorMessage(hr));
		}
	}
}


WinTaskbar::WinTaskbar(XMainWindow* window) {
	play_icon = qTheme.fontIcon(Glyphs::ICON_PLAY_LIST_PLAY);
	pause_icon = qTheme.fontIcon(Glyphs::ICON_PLAY_LIST_PAUSE);
	stop_play_icon = qTheme.fontIcon(Glyphs::ICON_STOP_PLAY);
	seek_forward_icon = qTheme.fontIcon(Glyphs::ICON_PLAY_FORWARD);
	seek_backward_icon = qTheme.fontIcon(Glyphs::ICON_PLAY_BACKWARD);

	auto hr = ::CoCreateInstance(CLSID_TaskbarList,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList4,
		reinterpret_cast<void**>(&taskbar_list_));

	if (FAILED(hr)) {
		throw Exception();
	}

	hr = taskbar_list_->HrInit();
	if (FAILED(hr)) {
		throw Exception();
	}

	if (MSG_TaskbarButtonCreated == WM_NULL) {
		MSG_TaskbarButtonCreated = ::RegisterWindowMessageW(L"TaskbarButtonCreated");
	}
	setWindow(window);
}

WinTaskbar::~WinTaskbar() = default;

//void setMicaEffect(HWND hwnd, bool is_dark_mode = false, bool is_alt = false) {
//	auto margins = MARGINS(-1, -1, -1, -1);
//	if (FAILED(DWM_DLL.DwmExtendFrameIntoClientArea(hwnd, &margins))) {
//		XAMP_LOG_DEBUG("DwmSetWindowAttribute return failure!");
//	}
//
//	WINDOWCOMPOSITIONATTRIBDATA win_comp_attr_data;
//	win_comp_attr_data.Attribute = WINDOWCOMPOSITIONATTRIB::WCA_ACCENT_POLICY;
//
//	ACCENT_POLICY accent_policy;
//	accent_policy.AccentState = ACCENT_STATE::ACCENT_ENABLE_HOSTBACKDROP;
//
//	win_comp_attr_data.SizeOfData = sizeof(ACCENT_POLICY);
//	win_comp_attr_data.Data = &accent_policy;
//
//	if (!USER32_DLL.SetWindowCompositionAttribute(hwnd, &win_comp_attr_data)) {
//		XAMP_LOG_DEBUG("SetWindowCompositionAttribute return failure!");
//	}
//
//	if (is_dark_mode) {
//		win_comp_attr_data.Attribute = WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS;
//		if (!USER32_DLL.SetWindowCompositionAttribute(hwnd, &win_comp_attr_data)) {
//			XAMP_LOG_DEBUG("SetWindowCompositionAttribute return failure!");
//		}
//	}
//
//	constexpr auto DWMWA_SYSTEMBACKDROP_TYPE = 38;
//	DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_MAINWINDOW;
//
//	if (FAILED(DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop)))) {
//		XAMP_LOG_DEBUG("DwmSetWindowAttribute return failure!");
//	}
//
//	const BOOL dark = is_dark_mode;
//	if (FAILED(DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark)))) {
//		XAMP_LOG_DEBUG("DwmSetWindowAttribute return failure!");
//	}
//}

void setMicaEffect(QWidget* window) {
	const auto hwnd = reinterpret_cast<HWND>(window->winId());
	//setMicaEffect(hwnd);
}

void WinTaskbar::setWindow(QWidget* window) {
	window_ = window;
	QCoreApplication::instance()->installNativeEventFilter(this);
	if (window_->isVisible()) {
		updateProgressIndicator();
		updateOverlay();
	}

	constexpr BOOL enable = TRUE;
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());

	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_HAS_ICONIC_BITMAP, &enable, sizeof(enable));
	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &enable, sizeof(enable));
	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_DISALLOW_PEEK, &enable, sizeof(enable));
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

void WinTaskbar::initialToolbarButtons() {
	for (int index = 0; index < kWinThumbbarButtonSize; index++) {
		buttons[index].iId = IDTB_FIRST + index;
		buttons[index].iBitmap = index;
		buttons[index].dwMask = THB_BITMAP | THB_FLAGS | THB_TOOLTIP;
		buttons[index].dwFlags = THBF_ENABLED;
	}

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	const auto hr = taskbar_list_->ThumbBarAddButtons(hwnd, kWinThumbbarButtonSize, buttons.data());
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("ThumbBarAddButtons return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::createToolbarImages() {
	QPixmap img;
	QBitmap mask;

	HImageHandleListHandle himl(COMCTL32_DLL.ImageList_Create(20, 20, ILC_COLOR32, 4, 0));

	img = seek_backward_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl.get(), qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	img = play_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl.get(), qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	img = seek_forward_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl.get(), qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	img = pause_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl.get(), qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	const auto hr = taskbar_list_->ThumbBarSetImageList(hwnd, himl.get());
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("ThumbBarSetImageList return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::setIconicThumbnail(const QPixmap& image) {
	if (!window_) {
		return;
	}
	thumbnail_ = image;
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	DWM_DLL.DwmInvalidateIconicBitmaps(hwnd);
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
		icon_handle.reset(overlay_icon_.pixmap(getWin32IconSize()).toImage().toHICON());

		if (!icon_handle) {
			icon_handle.reset(static_cast<HICON>(::LoadImageW(nullptr, IDI_APPLICATION, IMAGE_ICON, SM_CXSMICON, SM_CYSMICON, LR_SHARED)));
		}
	}

	if (icon_handle) {
		auto hr = taskbar_list_->SetOverlayIcon(hwnd, icon_handle.get(), description.c_str());
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("UpdateOverlay return failure! {}", GetPlatformErrorMessage(hr));
		}
	}
}

bool WinTaskbar::nativeEventFilter(const QByteArray& event_type, void* message, qintptr* result) {
	const auto* msg = static_cast<MSG*>(message);
	if (msg->message == MSG_TaskbarButtonCreated) {
		updateProgressIndicator();
		updateOverlay();
		createToolbarImages();
		initialToolbarButtons();
		return true;
	}

	switch (msg->message) {
	case WM_DWMSENDICONICTHUMBNAIL:
		updateIconicThumbnail(QSize(HIWORD(msg->lParam), LOWORD(msg->lParam)), msg->hwnd, thumbnail_);
		return true;
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP: 
		updateLiveThumbnail(msg->hwnd, window_->grab());
		return true;
	case WM_COMMAND:
	{
		const int button_id = LOWORD(msg->wParam) - IDTB_FIRST;

		if ((button_id >= 0) && (button_id < kWinThumbbarButtonSize)) {
			if (button_id == ToolButton_Play) {
				if (buttons[1].iBitmap == ToolButton_Play) {
					buttons[1].iBitmap = ToolButton_Pause;
					emit pauseClicked();
				}
				else {
					buttons[1].iBitmap = ToolButton_Play;
					emit playClicked();
				}

				const auto hwnd = reinterpret_cast<HWND>(window_->winId());
				const auto hr = taskbar_list_->ThumbBarUpdateButtons(hwnd, kWinThumbbarButtonSize, buttons.data());
				if (FAILED(hr)) {
					XAMP_LOG_ERROR("ThumbBarUpdateButtons return failure! {}", GetPlatformErrorMessage(hr));
				}
			}
			else {
				if (button_id == ToolButton_Forward) {
					emit forwardClicked();
				}
				if (button_id == ToolButton_Backward) {
					emit backwardClicked();
				}
			}
			return true;
		}
	}
	break;
	default:
		break;
	}
	return false;
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
	updateOverlay();
}

void WinTaskbar::setTaskbarPlayerPaused() {
	overlay_icon_ = pause_icon;
	state_ = TASKBAR_PROCESS_STATE_PAUSED;
	updateProgressIndicator();
	updateOverlay();
}

void WinTaskbar::setTaskbarPlayerPlaying() {
	resetTaskbarProgress();
}

void WinTaskbar::setTaskbarPlayerStop() {
	overlay_icon_ = stop_play_icon;
	state_ = TASKBAR_PROCESS_STATE_ERROR;
	updateProgressIndicator();
	updateOverlay();
}
#endif