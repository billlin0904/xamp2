#include <widget/win32/wintaskbar.h>
#include <widget/xmainwindow.h>
#include <widget/image_utiltis.h>
#include <widget/widget_shared.h>
#include <base/dll.h>

#include <xampplayer.h>

#include <QWindow>
#include <QCoreApplication>

#include <qt_windows.h>
#include <dwmapi.h>

#include <Windows.h>
#include <shobjidl.h>

enum HBitmapFormat {
	HBitmapNoAlpha,
	HBitmapPremultipliedAlpha,
	HBitmapAlpha
};

Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap& p, int hbitmapFormat = HBitmapFormat::HBitmapNoAlpha);

namespace win32 {
	namespace {		
		class DwmapiLib {
		public:
			DwmapiLib()
				: module_(OpenSharedLibrary("dwmapi"))
				, XAMP_LOAD_DLL_API(DwmInvalidateIconicBitmaps)
				, XAMP_LOAD_DLL_API(DwmSetWindowAttribute)
				, XAMP_LOAD_DLL_API(DwmSetIconicThumbnail) {
			}

			XAMP_DISABLE_COPY(DwmapiLib)

		private:
			SharedLibraryHandle module_;

		public:
			XAMP_DECLARE_DLL_NAME(DwmInvalidateIconicBitmaps);
			XAMP_DECLARE_DLL_NAME(DwmSetWindowAttribute);
			XAMP_DECLARE_DLL_NAME(DwmSetIconicThumbnail);
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
		
		static UINT MSG_TaskbarButtonCreated = WM_NULL;

		enum {
			ToolButton_Backward = 0,
			ToolButton_Play,
			ToolButton_Forward,
			ToolButton_Pause,
		};
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

	auto hr = taskbar_list_->HrInit();
	if (FAILED(hr)) {
		XAMP_LOG_DEBUG("HrInit return failure! {}", GetPlatformErrorMessage(hr));
	}

	MSG_TaskbarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");
	SetWindow(window);
}

WinTaskbar::~WinTaskbar() = default;

void WinTaskbar::SetWindow(QWidget* window) {
	window->removeEventFilter(this);
	window_ = window;

	QCoreApplication::instance()->installNativeEventFilter(this);

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

void WinTaskbar::InitialToolbarButtons() {
	for (int index = 0; index < kWinThumbbarButtonSize; index++) {
		buttons[index].iId = IDTB_FIRST + index;
		buttons[index].iBitmap = index;
		buttons[index].dwMask = (THUMBBUTTONMASK)(THB_BITMAP | THB_FLAGS | THB_TOOLTIP);
		buttons[index].dwFlags = (THUMBBUTTONFLAGS)(THBF_ENABLED);
	}

	const auto hwnd = reinterpret_cast<HWND>(window_->winId());
	auto hr = taskbar_list_->ThumbBarAddButtons(hwnd, kWinThumbbarButtonSize, buttons.data());
	if (FAILED(hr)) {
		XAMP_LOG_DEBUG("ThumbBarAddButtons return failure! {}", GetPlatformErrorMessage(hr));
	}
}

void WinTaskbar::CreateToolbarImages() {
	QPixmap img;
	QBitmap mask;

	HIMAGELIST himl = COMCTL32_DLL.ImageList_Create(20, 20, ILC_COLOR32, 4, 0);

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
		XAMP_LOG_DEBUG("ThumbBarSetImageList return failure! {}", GetPlatformErrorMessage(hr));
	}

	COMCTL32_DLL.ImageList_Destroy(himl);
}

void WinTaskbar::SetIconicThumbnail(const QPixmap& image) {
	if (!window_) {
		return;
	}
	thumbnail_ = image;	
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
			XAMP_LOG_DEBUG("UpdateOverlay return failure! {}", GetPlatformErrorMessage(hr));
		}
	}	

	if (icon_handle)
		::DestroyIcon(icon_handle);
}

bool WinTaskbar::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
	MSG* msg = reinterpret_cast<MSG*>(message);
	if (msg->message == MSG_TaskbarButtonCreated) {
		UpdateProgressIndicator();
		UpdateOverlay();
		CreateToolbarImages();
		InitialToolbarButtons();
	}
	else {
		switch (msg->message) {
		case WM_COMMAND:
		{
			int buttonId = LOWORD(msg->wParam) - IDTB_FIRST;

			if ((buttonId >= 0) && (buttonId < kWinThumbbarButtonSize)) {
				if (buttonId == ToolButton_Play) {
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
						XAMP_LOG_DEBUG("ThumbBarUpdateButtons return failure! {}", GetPlatformErrorMessage(hr));
					}
				}
				else {
					if (buttonId == ToolButton_Forward) {
						emit ForwardClicked();
					}
					if (buttonId == ToolButton_Backward) {
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

}

