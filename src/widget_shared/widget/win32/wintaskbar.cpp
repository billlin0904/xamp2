#include <widget/win32/wintaskbar.h>
#include <widget/xmainwindow.h>
#include <widget/image_utiltis.h>
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

Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap& p, int hbitmapFormat = HBitmapFormat::HBitmapNoAlpha);

namespace {
	class DwmapiLib {
	public:
		DwmapiLib()
			: module_(OpenSharedLibrary("dwmapi"))
			, XAMP_LOAD_DLL_API(DwmInvalidateIconicBitmaps)
			, XAMP_LOAD_DLL_API(DwmSetWindowAttribute)
			, XAMP_LOAD_DLL_API(DwmSetIconicThumbnail)
			, XAMP_LOAD_DLL_API(DwmSetIconicLivePreviewBitmap) {
		}

		XAMP_DISABLE_COPY(DwmapiLib)

	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(DwmInvalidateIconicBitmaps);
		XAMP_DECLARE_DLL_NAME(DwmSetWindowAttribute);
		XAMP_DECLARE_DLL_NAME(DwmSetIconicThumbnail);
		XAMP_DECLARE_DLL_NAME(DwmSetIconicLivePreviewBitmap);
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

	TBPFLAG GetWin32ProgressState(TaskbarProgressState state) {
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

	int GetWin32IconSize() {
		return ::GetSystemMetrics(SM_CXSMICON);
	}

#define DWM_DLL Singleton<DwmapiLib>::GetInstance()
#define COMCTL32_DLL Singleton<Comctl32Lib>::GetInstance()
#define IDTB_FIRST 3000

	UINT MSG_TaskbarButtonCreated = WM_NULL;

	enum {
		ToolButton_Backward = 0,
		ToolButton_Play,
		ToolButton_Forward,
		ToolButton_Pause,
	};

	void UpdateLiveThumbnail(const MSG* msg, const QPixmap& thumbnail) {
		RECT rect;
		GetClientRect(msg->hwnd, &rect);
		const QSize max_size(rect.right, rect.bottom);
		POINT offset = { 0, 0 };
		const HBITMAP bitmap = qt_pixmapToWinHBITMAP(image_utils::ResizeImage(thumbnail, max_size, true));
		const auto hr = DWM_DLL.DwmSetIconicLivePreviewBitmap(msg->hwnd, bitmap, &offset, 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicLivePreviewBitmap return failure! {}", GetPlatformErrorMessage(hr));
		}
		::DeleteObject(bitmap);
	}

	void UpdateIconicThumbnail(const MSG* msg, const QPixmap& thumbnail) {
		const QSize max_size(HIWORD(msg->lParam), LOWORD(msg->lParam));
		XAMP_LOG_DEBUG("{} {}", HIWORD(msg->lParam), LOWORD(msg->lParam));
		const HBITMAP bitmap = qt_pixmapToWinHBITMAP(image_utils::ResizeImage(thumbnail, max_size, true));
		const auto hr = DWM_DLL.DwmSetIconicThumbnail(msg->hwnd, bitmap, 0);
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("DwmSetIconicThumbnail return failure! {}", GetPlatformErrorMessage(hr));
		}
		::DeleteObject(bitmap);
	}
}

WinTaskbar::WinTaskbar(XMainWindow* window) {
	play_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_LIST_PLAY);
	pause_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_LIST_PAUSE);
	stop_play_icon = qTheme.GetFontIcon(Glyphs::ICON_STOP_PLAY);
	seek_forward_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_FORWARD);
	seek_backward_icon = qTheme.GetFontIcon(Glyphs::ICON_PLAY_BACKWARD);

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

	MSG_TaskbarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");
	SetWindow(window);
}

WinTaskbar::~WinTaskbar() = default;

void WinTaskbar::SetWindow(QWidget* window) {
	window_ = window;
	QCoreApplication::instance()->installNativeEventFilter(this);
	if (window_->isVisible()) {
		UpdateProgressIndicator();
		UpdateOverlay();
	}

	BOOL enable = TRUE;
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_HAS_ICONIC_BITMAP, &enable, sizeof(enable));
	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &enable, sizeof(enable));
	DWM_DLL.DwmSetWindowAttribute(hwnd, DWMWA_DISALLOW_PEEK, &enable, sizeof(enable));
	DWM_DLL.DwmInvalidateIconicBitmaps(hwnd);
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
		XAMP_LOG_ERROR("UpdateProgressIndicator return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::InitialToolbarButtons() {
	for (int index = 0; index < kWinThumbbarButtonSize; index++) {
		buttons[index].iId = IDTB_FIRST + index;
		buttons[index].iBitmap = index;
		buttons[index].dwMask = THB_BITMAP | THB_FLAGS | THB_TOOLTIP;
		buttons[index].dwFlags = THBF_ENABLED;
	}

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	auto hr = taskbar_list_->ThumbBarAddButtons(hwnd, kWinThumbbarButtonSize, buttons.data());
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("ThumbBarAddButtons return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::CreateToolbarImages() {
	QPixmap img;
	QBitmap mask;

	const HIMAGELIST himl = COMCTL32_DLL.ImageList_Create(20, 20, ILC_COLOR32, 4, 0);

	img = seek_backward_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl, qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	img = play_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl, qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	img = seek_forward_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl, qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	img = pause_icon.pixmap(20);
	mask = img.createMaskFromColor(Qt::transparent);
	COMCTL32_DLL.ImageList_Add(himl, qt_pixmapToWinHBITMAP(img, HBitmapPremultipliedAlpha), qt_pixmapToWinHBITMAP(mask));

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	auto hr = taskbar_list_->ThumbBarSetImageList(hwnd, himl);
	if (FAILED(hr)) {
		XAMP_LOG_ERROR("ThumbBarSetImageList return failure! {}", GetPlatformErrorMessage(hr));
	}

	COMCTL32_DLL.ImageList_Destroy(himl);
}

void WinTaskbar::SetIconicThumbnail(const QPixmap& image) {
	if (!window_) {
		return;
	}
	thumbnail_ = image;
	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	DWM_DLL.DwmInvalidateIconicBitmaps(hwnd);
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

	if (icon_handle != nullptr) {
		auto hr = taskbar_list_->SetOverlayIcon(hwnd, icon_handle, description.c_str());
		if (FAILED(hr)) {
			XAMP_LOG_ERROR("UpdateOverlay return failure! {}", GetPlatformErrorMessage(hr));
		}
	}	

	if (icon_handle)
		::DestroyIcon(icon_handle);
}

bool WinTaskbar::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
	auto* msg = reinterpret_cast<MSG*>(message);
	if (msg->message == MSG_TaskbarButtonCreated) {
		UpdateProgressIndicator();
		UpdateOverlay();
		CreateToolbarImages();
		InitialToolbarButtons();
	}
	else {
		switch (msg->message) {
		case WM_DWMSENDICONICTHUMBNAIL:
			if (!thumbnail_.isNull()) {
				UpdateIconicThumbnail(msg, thumbnail_);
			}
			break;
		case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
			UpdateLiveThumbnail(msg, window_->grab());
			break;
		case WM_COMMAND:
		{
			const int button_id = LOWORD(msg->wParam) - IDTB_FIRST;

			if ((button_id >= 0) && (button_id < kWinThumbbarButtonSize)) {
				if (button_id == ToolButton_Play) {
					if (buttons[1].iBitmap == ToolButton_Play) {
						buttons[1].iBitmap = ToolButton_Pause;
						emit PauseClicked();
					}
					else {
						buttons[1].iBitmap = ToolButton_Play;
						emit PlayClicked();
					}

					const auto hwnd = reinterpret_cast<HWND>(window_->winId());
					auto hr = taskbar_list_->ThumbBarUpdateButtons(hwnd, kWinThumbbarButtonSize, buttons.data());
					if (FAILED(hr)) {
						XAMP_LOG_ERROR("ThumbBarUpdateButtons return failure! {}", GetPlatformErrorMessage(hr));
					}
				}
				else {
					if (button_id == ToolButton_Forward) {
						emit ForwardClicked();
					}
					if (button_id == ToolButton_Backward) {
						emit BackwardClicked();
					}
				}				
			}
		}
			break;
		}
	}
	return false;
}

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
