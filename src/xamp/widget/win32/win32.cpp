#include <widget/widget_shared.h>
#include <widget/win32/win32.h>

#if defined(Q_OS_WIN)

#include <dwmapi.h>
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
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_INVALID_STATE = 4
} ACCENT_STATE;

typedef enum ACCENT_FLAGS {
	DrawLeftBorder = 0x20,
	DrawTopBorder = 0x40,
	DrawRightBorder = 0x80,
	DrawBottomBorder = 0x100,
	DrawAllBorders = (DrawLeftBorder | DrawTopBorder | DrawRightBorder | DrawBottomBorder)
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

typedef BOOL(WINAPI* pfnGetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

WINUSERAPI
BOOL
WINAPI
SetWindowCompositionAttribute(
	_In_ HWND hWnd,
	_Inout_ WINDOWCOMPOSITIONATTRIBDATA* pAttrData);

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

void setBlurMaterial(HWND hWnd, bool enable) {
	try {
		auto user32_module = LoadModule("user32.dll");
		XAMP_DECLARE_DLL(SetWindowCompositionAttribute) SetWindowCompositionAttribute {
			user32_module, "SetWindowCompositionAttribute"
		};

		if (SetWindowCompositionAttribute) {
			ACCENT_POLICY policy = { enable ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED, 0, 0, 0 };
			WINDOWCOMPOSITIONATTRIBDATA data;
			data.Attrib = WCA_ACCENT_POLICY;
			data.pvData = &policy;
			data.cbData = sizeof(policy);
			SetWindowCompositionAttribute(hWnd, &data);
		}
	}
	catch (...) {
	}
}

namespace win32 {
void setBlurMaterial(const QWidget* widget, bool enable) {
	if (enable) {
		auto hwnd = reinterpret_cast<HWND>(widget->winId());
		setBlurMaterial(hwnd, enable);
	}	
}

void setWinStyle(const QWidget* widget) {
	auto hwnd = reinterpret_cast<HWND>(widget->winId());

	auto dwm_module = LoadModule("dwmapi.dll");

	XAMP_DECLARE_DLL(DwmIsCompositionEnabled) DwmIsCompositionEnabled {
		dwm_module, "DwmIsCompositionEnabled"
	};

	XAMP_DECLARE_DLL(DwmSetWindowAttribute) DwmSetWindowAttribute {
		dwm_module, "DwmSetWindowAttribute"
	};

	XAMP_DECLARE_DLL(DwmExtendFrameIntoClientArea) DwmExtendFrameIntoClientArea {
		dwm_module, "DwmExtendFrameIntoClientArea"
	};

	XAMP_DECLARE_DLL(DwmSetPresentParameters) DwmSetPresentParameters {
		dwm_module, "DwmSetPresentParameters"
	};

	BOOL is_dwm_enable = false;
	DwmIsCompositionEnabled(&is_dwm_enable);

	if (is_dwm_enable) {
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

		MARGINS borderless = { 0, 0, 0, 0 };
		DwmExtendFrameIntoClientArea(hwnd, &borderless);

		DWM_PRESENT_PARAMETERS dpp{ 0 };
		dpp.cbSize = sizeof(dpp);
		dpp.fQueue = TRUE;
		dpp.cBuffer = 2;
		dpp.fUseSourceRate = FALSE;
		dpp.cRefreshesPerFrame = 1;
		dpp.eSampling = DWM_SOURCE_FRAME_SAMPLING_POINT;
		DwmSetPresentParameters(hwnd, &dpp);
	}

	auto style = ::GetWindowLong(hwnd, GWL_STYLE);
	::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
}
}

#endif
