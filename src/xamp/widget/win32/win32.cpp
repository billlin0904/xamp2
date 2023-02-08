#include <widget/win32/win32.h>

#include <QWidget>
#include <QStyle>
#include <QOperatingSystemVersion>
#include <QApplication>
#include <QLayout>
#include <QObject>
#include <qevent.h>

#include <base/singleton.h>
#include <base/platform.h>
#include <base/platfrom_handle.h>
#include <base/uuid.h>
#include <base/siphash.h>
#include <base/logger_impl.h>

#include "xampplayer.h"
#include "thememanager.h"
#include <widget/xmainwindow.h>
#include <widget/widget_shared.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>

#include <dwmapi.h>
#include <unknwn.h>
#include <propvarutil.h>
#include <uxtheme.h>

#include <Windows.h>
#include <windowsx.h>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinThumbnailToolBar>
#include <QtWinExtras/QWinThumbnailToolButton>

#include <WinUser.h>
#include <wingdi.h>
#include <Dbt.h>

#include <base/google_siphash.h>
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
	WCA_LAST = 23,
	WCA_USEDARKMODECOLORS = 26,
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


// ordinal 132
WINUSERAPI
BOOL
WINAPI
ShouldAppsUseDarkMode();


// ordinal 138
WINUSERAPI
BOOL
WINAPI
ShouldSystemUseDarkMode();

// ordinal 133
WINUSERAPI
BOOL
WINAPI
AllowDarkModeForWindow(HWND hWnd, bool allow);

// ordinal 135, in win10 1809
WINUSERAPI
BOOL
WINAPI
AllowDarkModeForApp(bool allow);

enum PreferredAppMode {
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

// ordinal 135, in win10 1903
WINUSERAPI
BOOL
WINAPI
SetPreferredAppMode(PreferredAppMode appMode);

// ordinal 104
WINUSERAPI
void
WINAPI
RefreshImmersiveColorPolicyState();


namespace win32 {

class User32Lib {
public:
	User32Lib() 
		: module_(OpenSharedLibrary("user32"))
		, SetWindowCompositionAttribute(module_, "SetWindowCompositionAttribute") {
	}

	XAMP_DISABLE_COPY(User32Lib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL(SetWindowCompositionAttribute) SetWindowCompositionAttribute;
};

class DwmapiLib {
public:
	DwmapiLib()
		: module_(OpenSharedLibrary("dwmapi"))
		, DwmIsCompositionEnabled(module_, "DwmIsCompositionEnabled")
		, DwmSetWindowAttribute(module_, "DwmSetWindowAttribute")
		, DwmExtendFrameIntoClientArea(module_, "DwmExtendFrameIntoClientArea")
		, DwmSetPresentParameters(module_, "DwmSetPresentParameters")
		, DwmGetColorizationColor(module_, "DwmGetColorizationColor") {
	}

	XAMP_DISABLE_COPY(DwmapiLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(DwmIsCompositionEnabled);
	XAMP_DECLARE_DLL_NAME(DwmSetWindowAttribute);
	XAMP_DECLARE_DLL_NAME(DwmExtendFrameIntoClientArea);
	XAMP_DECLARE_DLL_NAME(DwmSetPresentParameters);
	XAMP_DECLARE_DLL_NAME(DwmGetColorizationColor);
};

class UxThemeLib {
public:
	static constexpr int32_t UXTHEME_SHOULDAPPSUSEDARKMODE_ORDINAL = 132;

	UxThemeLib()
		: module_(OpenSharedLibrary("uxtheme"))
		, AllowDarkModeForWindow(module_, "AllowDarkModeForWindow ", 133)
		, SetPreferredAppMode(module_, "SetPreferredAppMode", 135)
		, ShouldAppsUseDarkMode(module_, "ShouldSystemUseDarkMode", 132)
		, ShouldSystemUseDarkMode(module_, "ShouldAppsUseDarkMode", 138)
		, RefreshImmersiveColorPolicyState(module_, "RefreshImmersiveColorPolicyState", 104)
		, SetWindowTheme(module_, "SetWindowTheme") {
	}

	XAMP_DISABLE_COPY(UxThemeLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(AllowDarkModeForWindow);
	XAMP_DECLARE_DLL_NAME(SetPreferredAppMode);
	XAMP_DECLARE_DLL_NAME(ShouldAppsUseDarkMode);
	XAMP_DECLARE_DLL_NAME(ShouldSystemUseDarkMode);
	XAMP_DECLARE_DLL_NAME(RefreshImmersiveColorPolicyState);
	XAMP_DECLARE_DLL_NAME(SetWindowTheme);
};

#define DWMDLL Singleton<DwmapiLib>::GetInstance()
#define User32DLL Singleton<User32Lib>::GetInstance()
#define UX_THEME_DLL Singleton<UxThemeLib>::GetInstance()

// Ref : https://github.com/melak47/BorderlessWindow
enum class BorderlessWindowStyle : DWORD {
	WINDOWED_STYLE        = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
	AERO_BORDERLESS_STYLE = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
	BORDERLESS_STYLE      = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
};

static void SetBorderlessWindowStyle(const WId window_id, BorderlessWindowStyle new_style) {
	auto hwnd = reinterpret_cast<HWND>(window_id);

	auto old_style = static_cast<BorderlessWindowStyle>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
	if (new_style != old_style) {
		::SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG>(new_style));
		::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		::ShowWindow(hwnd, SW_SHOW);
	}
}

static uint32_t GetGradientColor(QColor const & color) {
	return color.red() << 0
		| color.green() << 8
		| color.blue() << 16
		| color.alpha() << 24;
}

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

static void SetAccentPolicy(HWND hwnd, bool enable, int animation_id) {
	QColor background_color(AppSettings::ValueAsString(kAppSettingBackgroundColor));
	background_color.setAlpha(50);

	ACCENT_POLICY policy = {
		enable ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED,
		ACCENT_FLAGS::DrawAllBorders,
		GetGradientColor(background_color),
		animation_id
	};

	WINDOWCOMPOSITIONATTRIBDATA data;
	data.Attrib = WCA_ACCENT_POLICY;
	data.pvData = &policy;
	data.cbData = sizeof policy;
	User32DLL.SetWindowCompositionAttribute(hwnd, &data);
}

void AddDwmMenuShadow(const WId window_id) {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	int value = DWMNCRENDERINGPOLICY::DWMNCRP_ENABLED;
	DWMDLL.DwmSetWindowAttribute(hwnd, DWMWINDOWATTRIBUTE::DWMWA_NCRENDERING_POLICY, &value, 4);
	MARGINS borderless = { 1, 1, 1, 1 };
	DWMDLL.DwmExtendFrameIntoClientArea(hwnd, &borderless);
}

void SetAccentPolicy(const WId window_id, bool enable, int animation_id) {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	SetAccentPolicy(hwnd, enable, animation_id);
}

void AddDwmShadow(const WId window_id) {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	MARGINS borderless = { 1, 1, 1, 1 };
	DWMDLL.DwmExtendFrameIntoClientArea(hwnd, &borderless);
}

QRect GetWindowRect(const WId window_id) {
	auto hwnd = reinterpret_cast<HWND>(window_id);
	RECT rect{ 0 };
	::GetWindowRect(hwnd, &rect);
	auto width = rect.right - rect.left;
	auto height = rect.bottom - rect.top;
	return QRect(rect.left, rect.top, width, height);
}

bool IsCompositionEnabled() {
	BOOL composition_enabled = FALSE;
	auto success = DWMDLL.DwmIsCompositionEnabled(&composition_enabled) == S_OK;
	return composition_enabled && success;
}

void SetWindowedWindowStyle(const WId window_id) {
	SetBorderlessWindowStyle(window_id, BorderlessWindowStyle::WINDOWED_STYLE);
}

void SetFramelessWindowStyle(const WId window_id) {
	auto new_style = BorderlessWindowStyle::WINDOWED_STYLE;
	if (IsCompositionEnabled()) {
		new_style = BorderlessWindowStyle::AERO_BORDERLESS_STYLE;
	} else {
		new_style = BorderlessWindowStyle::BORDERLESS_STYLE;
	}
	SetBorderlessWindowStyle(window_id, new_style);
}

QColor GetColorizationColor() {
	DWORD color = 0;
	BOOL opaque_blend = 0;
	DWMDLL.DwmGetColorizationColor(&color, &opaque_blend);
	return QColor(color >> 24, color >> 16, color >> 8, color);
}

bool IsDarkModeAppEnabled() {
	std::array<char, 4> buffer{ 0 };
	auto data_size = static_cast<DWORD>(buffer.size());
	const auto res = ::RegGetValueW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		L"AppsUseLightTheme",
		RRF_RT_REG_DWORD, // expected value type
		nullptr,
		buffer.data(),
		&data_size);
	if (res != ERROR_SUCCESS) {
		return false;
	}
	const auto i = buffer[3] << 24 |
		buffer[2] << 16 |
		buffer[1] << 8 |
		buffer[0];
	return i != 1;
}

union W128 {
	uint8_t  b[16];
	uint32_t w[4];
	uint64_t q[2];
	double   d[2];
};

// https://github.com/odzhan/polymutex
std::string GetRandomMutexName(const std::string& src_name) {
	W128 s{};
	PRNG prng;
	// Golden Ratio constant used for better hash scattering
	// See https://softwareengineering.stackexchange.com/a/402543
	static constexpr auto kGoldenRatio = 0x9e3779b9ull;
	s.w[0] = static_cast<uint32_t>(prng.NextInt32() * kGoldenRatio);
	s.w[1] = static_cast<uint32_t>(prng.NextInt32() * kGoldenRatio);
	//s.w[0] = 642678;
	//s.w[1] = 3449517;
	s.q[1] = GoogleSipHash<>::GetHash(src_name, s.w[0], s.w[1]);
	return Uuid(s.b, s.b + 16);
}

bool IsValidMutexName(const std::string& guid, const std::string& mutex_name) {
	Uuid parsed_uuid;
	if (Uuid::TryParseString(guid, parsed_uuid)) {
		const auto* r = reinterpret_cast<const W128*>(parsed_uuid.GetBytes().data());
		const auto hash = GoogleSipHash<>::GetHash(mutex_name, r->w[0], r->w[1]);
		return r->q[1] == hash;
	}
	return false;
}

}

