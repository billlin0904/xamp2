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
#include <widget/xwindow.h>
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

typedef struct _LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} LSA_UNICODE_STRING, * PLSA_UNICODE_STRING, UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, * POBJECT_NAME_INFORMATION;

typedef enum _POOL_TYPE {
	NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed,
	DontUseThisType,
	NonPagedPoolCacheAligned,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS,
	MaxPoolType,
	NonPagedPoolSession = 32,
	PagedPoolSession,
	NonPagedPoolMustSucceedSession,
	DontUseThisTypeSession,
	NonPagedPoolCacheAlignedSession,
	PagedPoolCacheAlignedSession,
	NonPagedPoolCacheAlignedMustSSession
} POOL_TYPE;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemHandleInformation = 16,
} SYSTEM_INFORMATION_CLASS;

typedef struct  _SYSTEM_HANDLE {
	ULONG       ProcessId;
	UCHAR       ObjectTypeNumber;
	UCHAR       Flags;
	USHORT      Handle;
	PVOID       Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, * PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG HandleCount;
	SYSTEM_HANDLE Handles[ANYSIZE_ARRAY];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllTypesInformation,
	ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

typedef struct  _OBJECT_BASIC_INFORMATION {
	ULONG           Attributes;
	ACCESS_MASK     GrantedAccess;
	ULONG           HandleCount;
	ULONG           PointerCount;
	ULONG           PagedPoolUsage;
	ULONG           NonPagedPoolUsage;
	ULONG           Reserved[3];
	ULONG           NameInformationLength;
	ULONG           TypeInformationLength;
	ULONG           SecurityDescriptorLength;
	LARGE_INTEGER   CreateTime;
} OBJECT_BASIC_INFORMATION, * POBJECT_BASIC_INFORMATION;

typedef struct  _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING  Name;
	ULONG           ObjectCount;
	ULONG           HandleCount;
	ULONG           Reserved1[4];
	ULONG           PeakObjectCount;
	ULONG           PeakHandleCount;
	ULONG           Reserved2[4];
	ULONG           InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG           ValidAccess;
	UCHAR           Unknown;
	BOOLEAN         MaintainHandleDatabase;
	POOL_TYPE       PoolType;
	ULONG           PagedPoolUsage;
	ULONG           NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

NTSTATUS NtQuerySystemInformation (
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID                    SystemInformation,
	ULONG                    SystemInformationLength,
	PULONG                   ReturnLength);

NTSTATUS NtDuplicateObject(
	HANDLE      SourceProcessHandle,
	HANDLE      SourceHandle,
	HANDLE      TargetProcessHandle,
	PHANDLE     TargetHandle,
	ACCESS_MASK DesiredAccess,
	ULONG       HandleAttributes,
	ULONG       Options);

NTSTATUS NtQueryObject(
	HANDLE                   Handle,
	OBJECT_INFORMATION_CLASS ObjectInformationClass,
	PVOID                    ObjectInformation,
	ULONG                    ObjectInformationLength,
	PULONG                   ReturnLength);

NTSTATUS NtClose(HANDLE Handle);

typedef struct _HANDLE_ENTRY_T {
	DWORD pid;                      // process id
	WCHAR objName[MAX_PATH];        // name of object
} HANDLE_ENTRY, * PHANDLE_ENTRY;

struct ProcessHeapDeleter final {
	static void* invalid() noexcept {
		return nullptr;
	}

	static void close(void* value) {
		::HeapFree(::GetProcessHeap(), 0, value);
	}
};

using ProcessHeap = UniqueHandle<PVOID, ProcessHeapDeleter>;

static void FreeProcessHeap(void* value) {
	::HeapFree(::GetProcessHeap(), 0, value);
}

static PVOID AllocProcessHeap(SIZE_T size) {
	return ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static ProcessHeap MakeProcessHeap(SIZE_T size) {
	return ProcessHeap(::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, size));
}

static LPVOID ReallocProcessHeap(LPVOID mem, SIZE_T size) {
	return ::HeapReAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size);
}

namespace win32 {

class NtDllLib {
public:
	NtDllLib()
		: module_(OpenSharedLibrary("ntdll"))
		, NtQuerySystemInformation(module_, "NtQuerySystemInformation")
		, NtQueryObject(module_, "NtQueryObject")
		, NtDuplicateObject(module_, "NtDuplicateObject") {
	}

	XAMP_DISABLE_COPY(NtDllLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(NtQuerySystemInformation);
	XAMP_DECLARE_DLL_NAME(NtQueryObject);
	XAMP_DECLARE_DLL_NAME(NtDuplicateObject);
};

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
#define NTDLL Singleton<NtDllLib>::GetInstance()
#define UX_THEME_DLL Singleton<UxThemeLib>::GetInstance()

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

WinTaskbar::WinTaskbar(XWindow* window, IXPlayerControlFrame* player_frame) {
	window_ = window;

	play_icon = qTheme.iconFromFont(Glyphs::ICON_PLAY);
	pause_icon = qTheme.iconFromFont(Glyphs::ICON_PAUSE);
	stop_play_icon = qTheme.iconFromFont(Glyphs::ICON_STOP_PLAY);
	seek_forward_icon = qTheme.iconFromFont(Glyphs::ICON_PLAY_FORWARD);
	seek_backward_icon = qTheme.iconFromFont(Glyphs::ICON_PLAY_BACKWARD);

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
		&IXPlayerControlFrame::playOrPause);

	auto* forward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
	forward_tool_button->setIcon(seek_forward_icon);
	(void)QObject::connect(forward_tool_button,
		&QWinThumbnailToolButton::clicked,
		player_frame,
		&IXPlayerControlFrame::playNext);

	auto* backward_tool_button = new QWinThumbnailToolButton(thumbnail_tool_bar_.get());
	backward_tool_button->setIcon(seek_backward_icon);
	(void)QObject::connect(backward_tool_button,
		&QWinThumbnailToolButton::clicked,
		player_frame,
		&IXPlayerControlFrame::playPrevious);

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

void setTitleBarColor(const WId window_id, ThemeColor theme_color) noexcept {
	// https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
	// Undocumented in Windows 10 SDK
	// (can be used by setting value as dwAttribute as 20),
	// value was 19 pre Windows 10 20H1 Update).

	constexpr DWORD kDWMWA_USE_IMMERSIVE_DARK_MODE = 20;
	constexpr DWORD kDWMWA_CAPTION_COLOR = 35;

	constexpr DWORD kDWMWA_MICA_EFFECT = 1029;

	constexpr int kWindows11MajorVersion = 10;
	constexpr int kWindows11MicroVersion = 22000;

	auto hwnd = reinterpret_cast<HWND>(window_id);
	auto color = qTheme.backgroundColor();

	const auto os_ver = QOperatingSystemVersion::current();
	const auto major_version = os_ver.majorVersion();
	const auto micro_version = os_ver.microVersion();

	switch (theme_color) {
	case ThemeColor::DARK_THEME:
		if (major_version == kWindows11MajorVersion && micro_version < kWindows11MicroVersion) {
			BOOL value = TRUE;
			DWMDLL.DwmSetWindowAttribute(hwnd,
				kDWMWA_USE_IMMERSIVE_DARK_MODE,
				&value,
				sizeof(value));
		} else {
			int use_mica = 1;
			DWMDLL.DwmSetWindowAttribute(hwnd,
				kDWMWA_MICA_EFFECT,
				&use_mica,
				sizeof(use_mica));

			int r = 0, g = 0, b = 0;
			color.getRgb(&r, &g, &b);
			COLORREF colorref = RGB(r, g, b);
			DWMDLL.DwmSetWindowAttribute(hwnd,
				kDWMWA_CAPTION_COLOR,
				&colorref,
				sizeof(COLORREF));
		}
		break;
	case ThemeColor::LIGHT_THEME:
		if (major_version == kWindows11MajorVersion && micro_version < kWindows11MicroVersion) {
			BOOL value = TRUE;

			DWMDLL.DwmSetWindowAttribute(hwnd,
				0,
				&value,
				sizeof(value));
		}
		else {
			int use_mica = 1;
			DWMDLL.DwmSetWindowAttribute(hwnd,
				kDWMWA_MICA_EFFECT,
				&use_mica,
				sizeof(use_mica));

			int r = 0, g = 0, b = 0;
			color.getRgb(&r, &g, &b);
			COLORREF colorref = RGB(r, g, b);
			DWMDLL.DwmSetWindowAttribute(hwnd,
				kDWMWA_CAPTION_COLOR,
				&colorref,
				sizeof(COLORREF));
		}
		break;
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
	std::array<char, 4> buffer{ 0 };
	auto data_size = static_cast<DWORD>(buffer.size());
	auto res = ::RegGetValueW(
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
std::string getRandomMutexName(const std::string& src_name) {
	W128 s{};
	PRNG prng;
	// Golden Ratio constant used for better hash scattering
	// See https://softwareengineering.stackexchange.com/a/402543
	static constexpr auto kGoldenRatio = 0x9e3779b9;
	s.w[0] = prng.NextInt32() * kGoldenRatio;
	s.w[1] = prng.NextInt32() * kGoldenRatio;
	s.q[1] = SipHash::GetHash(s.w[0], s.w[1], src_name);
	return Uuid(s.b, s.b + 16);
}

bool isRunning(const std::string& mutex_name) {
#define STATUS_INFO_LEN_MISMATCH NTSTATUS(0xC0000004)
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
	EnablePrivilege("SeDebugPrivilege", true);

	NTSTATUS status = 0;
	ULONG len = 0;
	ULONG total = 0;

	static constexpr size_t kSysHandleInfoHeapSize = 65535;

	auto sys_handle_info_heap = AllocProcessHeap(kSysHandleInfoHeapSize);
	if (!sys_handle_info_heap) {
		return false;
	}

	XAMP_LOG_DEBUG("Grabbing handles...");

	do {
		len += kSysHandleInfoHeapSize;
		sys_handle_info_heap = ReallocProcessHeap(sys_handle_info_heap, len);

		if (!sys_handle_info_heap) {
			break;
		}

		status = NTDLL.NtQuerySystemInformation(
			SystemHandleInformation,
			sys_handle_info_heap,
			len,
			&total);
	} while (status == STATUS_INFO_LEN_MISMATCH);

	const ProcessHeap process_heap(sys_handle_info_heap);

	if (!NT_SUCCESS(status)) {
		return false;
	}

	const auto sys_handle_info = static_cast<PSYSTEM_HANDLE_INFORMATION>(process_heap.get());
	XAMP_LOG_DEBUG("done. Fetched {} handles allocate size: {}.", sys_handle_info->HandleCount, len);

	for (ULONG i = 0; i < sys_handle_info->HandleCount; i++) {
		if (sys_handle_info->Handles[i].ProcessId == 4) {
			continue;
		}

		WinHandle process(::OpenProcess(PROCESS_DUP_HANDLE,
			FALSE,
			sys_handle_info->Handles[i].ProcessId));

		if (!process) {
			continue;
		}

		HANDLE object_handle = nullptr;
		status = ::DuplicateHandle(process.get(),
			reinterpret_cast<HANDLE>(sys_handle_info->Handles[i].Handle), 
			GetCurrentProcess(),
			&object_handle, 
			0,
			FALSE, 
			DUPLICATE_SAME_ACCESS);
		if (!status) {
			continue;
		}

		//WinHandle duplicate_handle(object_handle);

		OBJECT_BASIC_INFORMATION obj_basic_info{0};
		status = NTDLL.NtQueryObject(object_handle,
			ObjectBasicInformation, 
			&obj_basic_info,
			sizeof(obj_basic_info),
			&len);
		if (!NT_SUCCESS(status)) {
			continue;
		}

		if (!obj_basic_info.NameInformationLength) {
			continue;
		}

		len = obj_basic_info.TypeInformationLength + 2;
		auto obj_type_info_heap = MakeProcessHeap(len);
		const auto obj_type_info = static_cast<POBJECT_TYPE_INFORMATION>(obj_type_info_heap.get());
		status = NTDLL.NtQueryObject(object_handle,
			ObjectTypeInformation,
			obj_type_info,
			len,
			&len);
		if (NT_SUCCESS(status)) {
			if (lstrcmpi(obj_type_info->Name.Buffer, L"Mutant") != 0) {
				continue;
			}
		}

		// Skip some objects to avoid getting stuck
		// see: https://github.com/adamdriscoll/PoshInternals/issues/7
		if (sys_handle_info->Handles[i].GrantedAccess    == 0x0012019f
			&& sys_handle_info->Handles[i].GrantedAccess != 0x00120189
			&& sys_handle_info->Handles[i].GrantedAccess != 0x120089
			&& sys_handle_info->Handles[i].GrantedAccess != 0x1A019F) {
			continue;
		}

		len = obj_basic_info.NameInformationLength + 2;
		auto obj_name_info_heap = MakeProcessHeap(len);
		const auto name_info = static_cast<POBJECT_NAME_INFORMATION>(obj_name_info_heap.get());
		status = NTDLL.NtQueryObject(object_handle,
			ObjectNameInformation,
			name_info,
			len,
			&len);
		if (!NT_SUCCESS(status)) {
			continue;
		}

		auto p = wcsrchr(name_info->Name.Buffer, L'\\');
		if (!p) {
			continue;
		}

		p = wcsrchr(p, L'_');
		if (!p) {
			continue;
		}

		p += 1;
		auto handle_mutex_name = String::ToLower(String::ToString(name_info->Name.Buffer));
		auto handle_mutex_uuid = String::ToLower(String::ToString(p));
		
		Uuid parsed_uuid;
		if (Uuid::TryParseString(handle_mutex_uuid, parsed_uuid)) {
			const auto* r = reinterpret_cast<const W128*>(parsed_uuid.GetBytes().data());
			const auto hash = SipHash::GetHash(r->w[0], r->w[1], mutex_name);
			XAMP_LOG_DEBUG("Found GUID => {}, k0:{:#04x}, k1:{:#04x}, q:{:#04x}, hash:{:#04x}",
				handle_mutex_uuid, static_cast<uint64_t>(r->w[0]), static_cast<uint64_t>(r->w[1]),
				r->q[1], hash);
			if (r->q[1] == hash) {
				return true;
			}
		}
	}
	return false;
}

}

