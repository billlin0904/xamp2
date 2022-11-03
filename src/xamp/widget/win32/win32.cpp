#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QStyle>

#include <base/singleton.h>
#include <base/platfrom_handle.h>

#if defined(Q_OS_WIN)
#include <dwmapi.h>
#include <unknwn.h>
#endif

#include "xampplayer.h"
#include "thememanager.h"
#include <widget/xwindow.h>
#include <widget/widget_shared.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/win32/win32.h>

#if defined(Q_OS_WIN)

#include <Windows.h>
#include <windowsx.h>
#include <QtWin>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>

#include <WinUser.h>
#include <wingdi.h>

#include <base/dll.h>

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
	WCA_LAST = 23
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef enum _ACCENT_STATE
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT,
	ACCENT_ENABLE_TRANSPARENTGRADIENT,
	ACCENT_ENABLE_BLURBEHIND,
	ACCENT_ENABLE_ACRYLICBLURBEHIND,
	ACCENT_ENABLE_HOSTBACKDROP,
	ACCENT_INVALID_STATE
} ACCENT_STATE;

typedef enum ACCENT_FLAGS {
	DrawLeftBorder = 0x20,
	DrawTopBorder = 0x40,
	DrawRightBorder = 0x80,
	DrawBottomBorder = 0x100,
	DrawAllBorders = DrawLeftBorder | DrawTopBorder | DrawRightBorder | DrawBottomBorder
} ACCENT_FLAGS;

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
GetWindowCompositionAttribute(
	_In_ HWND hWnd,
	_Inout_ WINDOWCOMPOSITIONATTRIBDATA* pAttrData);

WINUSERAPI
BOOL
WINAPI
SetWindowCompositionAttribute(
	_In_ HWND hWnd,
	_Inout_ WINDOWCOMPOSITIONATTRIBDATA* pAttrData);

namespace win32 {

class User32Lib {
public:
	User32Lib() 
		: module_(LoadModule("user32.dll"))
		, SetWindowCompositionAttribute(module_, "SetWindowCompositionAttribute") {
	}

	XAMP_DISABLE_COPY(User32Lib)

private:
	ModuleHandle module_;

public:
	XAMP_DECLARE_DLL(SetWindowCompositionAttribute) SetWindowCompositionAttribute;
};

class DwmapiLib {
public:
	DwmapiLib()
		: module_(LoadModule("dwmapi.dll"))
		, DwmIsCompositionEnabled(module_, "DwmIsCompositionEnabled")
		, DwmSetWindowAttribute(module_, "DwmSetWindowAttribute")
		, DwmExtendFrameIntoClientArea(module_, "DwmExtendFrameIntoClientArea")
		, DwmSetPresentParameters(module_, "DwmSetPresentParameters")
		, DwmGetColorizationColor(module_, "DwmGetColorizationColor") {
	}

	XAMP_DISABLE_COPY(DwmapiLib)

private:
	ModuleHandle module_;

public:
	XAMP_DECLARE_DLL(DwmIsCompositionEnabled) DwmIsCompositionEnabled;
	XAMP_DECLARE_DLL(DwmSetWindowAttribute) DwmSetWindowAttribute;
	XAMP_DECLARE_DLL(DwmExtendFrameIntoClientArea) DwmExtendFrameIntoClientArea;
	XAMP_DECLARE_DLL(DwmSetPresentParameters) DwmSetPresentParameters;
	XAMP_DECLARE_DLL(DwmGetColorizationColor) DwmGetColorizationColor;
};

#define DWMDLL Singleton<DwmapiLib>::GetInstance()
#define User32DLL Singleton<User32Lib>::GetInstance()

// Ref : https://github.com/melak47/BorderlessWindow
enum class BorderlessWindowStyle : DWORD {
	WINDOWED_STYLE        = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
	AERO_BORDERLESS_STYLE = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
	BORDERLESS_STYLE      = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
};

static void setBorderlessWindowStyle(const WId window_id, BorderlessWindowStyle new_style) noexcept {
	auto hwnd = reinterpret_cast<HWND>(window_id);

	auto old_style = static_cast<BorderlessWindowStyle>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
	if (new_style != old_style) {
		::SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG>(new_style));
		::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		::ShowWindow(hwnd, SW_SHOW);
	}
}

static uint32_t gradientColor(QColor const & color) {
	return color.red() << 0
		| color.green() << 8
		| color.blue() << 16
		| color.alpha() << 24;
}

WinTaskbar::WinTaskbar(XWindow* window, IXPlayerFrame* player_frame) {
	window_ = window;

	play_icon = qTheme.iconFromFont(IconCode::ICON_Play);
	pause_icon = qTheme.iconFromFont(IconCode::ICON_Pause);
	stop_play_icon = qTheme.iconFromFont(IconCode::ICON_StopPlay);
	seek_forward_icon = qTheme.iconFromFont(IconCode::ICON_PlayNext);
	seek_backward_icon = qTheme.iconFromFont(IconCode::ICON_PlayPrev);

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
		player_frame,
		&IXPlayerFrame::play);

	auto* forward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
	forward_tool_button->setIcon(seek_forward_icon);
	(void)QObject::connect(forward_tool_button,
		&QWinThumbnailToolButton::clicked,
		player_frame,
		&IXPlayerFrame::playNextClicked);

	auto* backward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
	backward_tool_button->setIcon(seek_backward_icon);
	(void)QObject::connect(backward_tool_button,
		&QWinThumbnailToolButton::clicked,
		player_frame,
		&IXPlayerFrame::playPreviousClicked);

	thumbnail_tool_bar_->addButton(backward_tool_button);
	thumbnail_tool_bar_->addButton(play_tool_button);
	thumbnail_tool_bar_->addButton(forward_tool_button);
}

WinTaskbar::~WinTaskbar() = default;

void WinTaskbar::setTaskbarProgress(const int32_t percent) {
	taskbar_progress_->setValue(percent);
}

void WinTaskbar::resetTaskbarProgress() {
	taskbar_progress_->reset();
	taskbar_progress_->setValue(0);
	taskbar_progress_->setRange(0, 100);
	taskbar_button_->setOverlayIcon(play_icon);
	taskbar_progress_->show();
}

void WinTaskbar::setTaskbarPlayingResume() {
	taskbar_button_->setOverlayIcon(play_icon);
	taskbar_progress_->resume();
}

void WinTaskbar::setTaskbarPlayerPaused() {
	taskbar_button_->setOverlayIcon(pause_icon);
	taskbar_progress_->pause();
}

void WinTaskbar::setTaskbarPlayerPlaying() {
	resetTaskbarProgress();
}

void WinTaskbar::setTaskbarPlayerStop() {
	taskbar_button_->setOverlayIcon(stop_play_icon);
	taskbar_progress_->hide();
}

void WinTaskbar::showEvent() {
	taskbar_button_->setWindow(window_->windowHandle());
}

void addDwmMenuShadow(const WId window_id) noexcept {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	int value = DWMNCRENDERINGPOLICY::DWMNCRP_ENABLED;
	DWMDLL.DwmSetWindowAttribute(hwnd, DWMWINDOWATTRIBUTE::DWMWA_NCRENDERING_POLICY, &value, 4);
	MARGINS borderless = { 1, 1, 1, 1 };
	DWMDLL.DwmExtendFrameIntoClientArea(hwnd, &borderless);
}

void setAccentPolicy(HWND hwnd, bool enable, int animation_id) noexcept {
	auto is_rs4_or_greater = false;

	ACCENT_STATE flags = (is_rs4_or_greater ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_ENABLE_BLURBEHIND);

	QColor background_color(AppSettings::getValueAsString(kAppSettingBackgroundColor));
	background_color.setAlpha(50);

	ACCENT_POLICY policy = {
		enable ? flags : ACCENT_DISABLED,
		ACCENT_FLAGS::DrawAllBorders,
		gradientColor(background_color),
		animation_id
	};

	WINDOWCOMPOSITIONATTRIBDATA data;
	data.Attrib = WCA_ACCENT_POLICY;
	data.pvData = &policy;
	data.cbData = sizeof policy;
	User32DLL.SetWindowCompositionAttribute(hwnd, &data);
}

void setAccentPolicy(const WId window_id, bool enable, int animation_id) noexcept {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	setAccentPolicy(hwnd, enable, animation_id);
}

void addDwmShadow(const WId window_id) noexcept {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	MARGINS borderless = { -1, -1, -1, -1 };
	DWMDLL.DwmExtendFrameIntoClientArea(hwnd, &borderless);
}

QRect windowRect(const WId window_id) noexcept {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	RECT rect{ 0 };
	if (::GetWindowRect(hwnd, &rect)) {
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		return QRect(rect.left, rect.top, width, height);
	}
	// todo: throw an system exception?
	return QRect();
}

bool compositionEnabled() noexcept {
	BOOL composition_enabled = FALSE;
	auto success = DWMDLL.DwmIsCompositionEnabled(&composition_enabled) == S_OK;
	return composition_enabled && success;
}

void setWindowedWindowStyle(const WId window_id) noexcept {
	setBorderlessWindowStyle(window_id, BorderlessWindowStyle::WINDOWED_STYLE);
}

void setFramelessWindowStyle(const WId window_id) noexcept {
	auto new_style = BorderlessWindowStyle::WINDOWED_STYLE;
	if (compositionEnabled()) {
		new_style = BorderlessWindowStyle::AERO_BORDERLESS_STYLE;
	} else {
		new_style = BorderlessWindowStyle::BORDERLESS_STYLE;
	}
	setBorderlessWindowStyle(window_id, new_style);
}

void setTitleBarColor(const WId window_id, QColor color) noexcept {
	// https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
	// Undocumented in Windows 10 SDK
	// (can be used by setting value as dwAttribute as 20),
	// value was 19 pre Windows 10 20H1 Update).

	constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
	constexpr DWORD DWMWA_BORDER_COLOR = 34;
	constexpr DWORD DWMWA_CAPTION_COLOR = 19;
	constexpr DWORD DWMWA_MICA_EFFECT = 1029;

	constexpr int kWindows11MajorVersion = 10;
	constexpr int kWindows11MicroVersion = 22000;

	auto hwnd = reinterpret_cast<HWND>(window_id);

	const auto os_ver = QOperatingSystemVersion::current();
	if (os_ver.majorVersion() == kWindows11MajorVersion && os_ver.microVersion() < kWindows11MicroVersion) {
		BOOL value = TRUE;

		DWMDLL.DwmSetWindowAttribute(hwnd,
			DWMWA_USE_IMMERSIVE_DARK_MODE,
			&value,
			sizeof(value));
	}
	else {
		int use_mica = 1;
		DWMDLL.DwmSetWindowAttribute(hwnd,
			DWMWA_MICA_EFFECT, 
			&use_mica,
			sizeof(use_mica));

		int r, g, b;
		color.getRgb(&r, &g, &b);
		COLORREF colorref = RGB(r, g, b);
		DWMDLL.DwmSetWindowAttribute(hwnd,
			DWMWA_CAPTION_COLOR,
			&colorref,
			sizeof(COLORREF));
	}
	
}

bool isWindowMaximized(const WId window_id) noexcept {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	return ::GetWindowLong(hwnd, GWL_STYLE) & WS_MINIMIZE;
}

QColor colorizationColor() noexcept {
	DWORD color = 0;
	BOOL opaque_blend = 0;
	DWMDLL.DwmGetColorizationColor(&color, &opaque_blend);
	return QColor(color >> 24, color >> 16, color >> 8, color);
}

bool isDarkModeAppEnabled() noexcept {
	auto buffer = std::vector<char>(4);
	auto data = static_cast<DWORD>(buffer.size() * sizeof(char));
	auto res = RegGetValueW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		L"AppsUseLightTheme",
		RRF_RT_REG_DWORD, // expected value type
		nullptr,
		buffer.data(),
		&data);
	if (res != ERROR_SUCCESS) {
		return false;
	}
	const auto i = buffer[3] << 24 |
		buffer[2] << 16 |
		buffer[1] << 8 |
		buffer[0];
	return i != 1;
}

}

#endif
