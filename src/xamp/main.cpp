#include <cstdio>

#include <base/logger.h>
#include <base/rng.h>
#include <base/dll.h>
#include <base/vmmemlock.h>
#include <base/stacktrace.h>

#include <player/audio_player.h>
#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>

#include <QMessageBox>
#include <QtWidgets/QApplication>

#include "thememanager.h"
#include "singleinstanceapplication.h"
#include "xamp.h"

static void setOrDefaultConfig() {
	AppSettings::loadIniFile(Q_UTF8("xamp.ini"));
	AppSettings::setDefaultValue(APP_SETTING_DEVICE_TYPE, Q_UTF8(""));
	AppSettings::setDefaultValue(APP_SETTING_DEVICE_ID, Q_UTF8(""));
	AppSettings::setDefaultValue(APP_SETTING_WIDTH, 600);
	AppSettings::setDefaultValue(APP_SETTING_HEIGHT, 500);
	AppSettings::setDefaultValue(APP_SETTING_VOLUME, 50);
	AppSettings::setDefaultValue(APP_SETTING_NIGHT_MODE, false);
	AppSettings::setDefaultValue(APP_SETTING_ORDER, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
	AppSettings::setDefaultValue(APP_SETTING_BACKGROUND_COLOR, QColor("#01121212"));
	AppSettings::setDefaultValue(APP_SETTING_ENABLE_BLUR, true);
}

static int excute(int argc, char* argv[]) {
	Logger::Instance()
#ifdef Q_OS_WIN
		.AddDebugOutputLogger()
#else
		.AddSink(std::make_shared<QDebugSink>())
#endif
		.AddFileLogger("xamp.log");

	XAMP_LOG_DEBUG("Logger init success.");

	StackTrace::RegisterAbortHandler();

	XAMP_LOG_DEBUG("RegisterAbortHandler success.");

	std::vector<ModuleHandle> preload_modules;

	try {
#ifdef Q_OS_WIN	
		// 如果沒有預先載入拖入檔案會爆音.
		preload_modules.emplace_back(LoadDll("psapi.dll"));
		preload_modules.emplace_back(LoadDll("comctl32.dll"));
		preload_modules.emplace_back(LoadDll("WindowsCodecs.dll"));
		// 為了效率考量.
		preload_modules.emplace_back(LoadDll("AUDIOKSE.dll"));
		preload_modules.emplace_back(LoadDll("avrt.dll"));
		AudioPlayer::LoadLib();
		VmMemLock::EnableLockMemPrivilege(true);
#else
		preload_modules.emplace_back(LoadDll("libchromaprint.dylib"));
		preload_modules.emplace_back(LoadDll("libbass.dylib"));
		AudioPlayer::LoadLib();
#endif
	}
	catch (const Exception& e) {
		QMessageBox::critical(nullptr, Q_UTF8("Load dll failure!"), QString::fromStdString(e.GetErrorMessage()));
		return -1;
	}

	XAMP_LOG_DEBUG("Preload dll success.");

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QApplication app(argc, argv);

	SingleInstanceApplication singleApp;
	if (!singleApp.attach(QCoreApplication::arguments())) {
		return -1;
	}

	XAMP_LOG_DEBUG("attach app success.");

	(void)RNG::Instance();
	(void)PixmapCache::instance();

	XAMP_LOG_DEBUG("PixmapCache init success.");
	
	try {
		Database::instance().open(Q_UTF8("xamp.db"));
	}
	catch (const std::exception& e) {
		XAMP_LOG_INFO("Initial database failure! {}", e.what());
		return -1;
	}

	XAMP_LOG_DEBUG("Database init success.");
	
	setOrDefaultConfig();

	XAMP_LOG_DEBUG("setOrDefaultConfig success.");

	Xamp win;
	win.init();
	win.show();
	return app.exec();
}

#ifdef XAMP_OS_WIN
static int tryExcute(int argc, char* argv[]) {
	DWORD code = 0;
	LPEXCEPTION_POINTERS info = nullptr;

	__try {
		return excute(argc, argv);
	}
	__except (code = GetExceptionCode(), info = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Exception code: 0x%08x", code);
		::MessageBoxA(nullptr, buffer, "Something wrong!", MB_OK);
		return -1;
	}
}
#else
static int tryExcute(int argc, char* argv[]) {
    return excute(argc, argv);
}
#endif

int main(int argc, char *argv[]) {
	return tryExcute(argc, argv);
}
