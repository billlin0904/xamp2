#include "xamp.h"

#include <iostream>
#include <QtWidgets/QApplication>

#if defined(Q_OS_WIN)
#pragma comment(lib, "Dwmapi.lib")

#include <dwmapi.h>
#include <base/windows_handle.h>

struct DwmMMCSSInit {
	DwmMMCSSInit() {
		::DwmEnableMMCSS(true);
	}

	~DwmMMCSSInit() {
		::DwmEnableMMCSS(false);
	}
};
#endif

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);	
#if defined(Q_OS_WIN)
	DwmMMCSSInit dwm_mmcss_init;
#endif
	Xamp win;
	win.show();
	return app.exec();
}
