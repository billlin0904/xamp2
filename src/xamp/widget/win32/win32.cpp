#include <widget/widget_shared.h>
#include <widget/appsettings.h>
#include <widget/win32/win32.h>

#if defined(Q_OS_WIN)

#include <winuser.h>
#include <wingdi.h>
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

typedef BOOL(WINAPI* pfnGetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

WINUSERAPI
BOOL
WINAPI
SetWindowCompositionAttribute(
	_In_ HWND hWnd,
	_Inout_ WINDOWCOMPOSITIONATTRIBDATA* pAttrData);

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

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

static uint32_t toABGR(QColor const & color) {
	return color.alpha() << 24
		| color.blue() << 16
		| color.green() << 8
		| color.red();
}

static QColor blendColor(const QColor& i_color1, const QColor& i_color2, double alpha) {
	return QColor(
		qRound(static_cast<qreal>(i_color1.red()) * (1.0 - alpha) + static_cast<qreal>(i_color2.red()) * alpha),
		qRound(static_cast<qreal>(i_color1.green()) * (1.0 - alpha) + static_cast<qreal>(i_color2.green()) * alpha),
		qRound(static_cast<qreal>(i_color1.blue()) * (1.0 - alpha) + static_cast<qreal>(i_color2.blue()) * alpha),
		qRound(static_cast<qreal>(i_color1.alpha()) * (1.0 - alpha) + static_cast<qreal>(i_color2.alpha()) * alpha)
	);
}

void setBlurMaterial(const QWidget* widget, bool enable) {
	auto hwnd = reinterpret_cast<HWND>(widget->winId());
	auto is_rs4_or_greater = true;

	ACCENT_STATE flags = (is_rs4_or_greater ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_ENABLE_BLURBEHIND);

	//DWORD wincolor = 0;
	//BOOL opaque = FALSE;
	//DWMDLL.DwmGetColorizationColor(&wincolor, &opaque);
	//auto background_color = QColor::fromRgba(wincolor);
	QColor background_color(AppSettings::getValueAsString(kAppSettingBackgroundColor));
	background_color.setAlpha(50);
	ACCENT_POLICY policy = {
		enable ? flags : ACCENT_DISABLED,
		0,
		toABGR(background_color),
		0
	};
	WINDOWCOMPOSITIONATTRIBDATA data;
	data.Attrib = WCA_ACCENT_POLICY;
	data.pvData = &policy;
	data.cbData = sizeof policy;
	User32DLL.SetWindowCompositionAttribute(hwnd, &data);
}

void drawDwmShadow(const QWidget* widget) {
	auto hwnd = reinterpret_cast<HWND>(widget->winId());
	/*auto policy = DWMNCRENDERINGPOLICY::DWMNCRP_ENABLED;
	DWMDLL.DwmSetWindowAttribute(hwnd, DWMWINDOWATTRIBUTE::DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));*/

	MARGINS borderless = { 0, 1, 0, 1 };
	DWMDLL.DwmExtendFrameIntoClientArea(hwnd, &borderless);
}

void setFramelessWindowStyle(const QWidget* widget) {
	auto hwnd = reinterpret_cast<HWND>(widget->winId());
	const DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
	::SetWindowLong(hwnd, GWL_STYLE, style | WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
	drawDwmShadow(widget);
}
}

#endif
