#include <base/logger.h>
#include <base/rng.h>
#include <base/dll.h>
#include <base/vmmemlock.h>

#include <player/audio_player.h>
#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>

#include <QtWidgets/QApplication>

#include "singleinstanceapplication.h"
#include "xamp.h"

static void loadAndDefaultAppConfig() {
	AppSettings::settings().loadIniFile(Q_UTF8("xamp.ini"));
	AppSettings::settings().setDefaultValue(APP_SETTING_DEVICE_TYPE, Q_UTF8(""));
	AppSettings::settings().setDefaultValue(APP_SETTING_DEVICE_ID, Q_UTF8(""));
	AppSettings::settings().setDefaultValue(APP_SETTING_WIDTH, 600);
	AppSettings::settings().setDefaultValue(APP_SETTING_HEIGHT, 500);
	AppSettings::settings().setDefaultValue(APP_SETTING_VOLUME, 50);
	AppSettings::settings().setDefaultValue(APP_SETTING_NIGHT_MODE, false);
	AppSettings::settings().setDefaultValue(APP_SETTING_ORDER, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
}

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN	
	std::vector<ModuleHandle> preload_modules;
	// �p�G�S���w�����J��J�ɮ׷|�z��.
	preload_modules.emplace_back(LoadDll("psapi.dll"));
	preload_modules.emplace_back(LoadDll("comctl32.dll"));
	preload_modules.emplace_back(LoadDll("WindowsCodecs.dll"));
	// ���F�Ĳv�Ҷq.
	preload_modules.emplace_back(LoadDll("chromaprint.dll"));
	preload_modules.emplace_back(LoadDll("bass.dll"));

	VmMemLock::EnableLockMemPrivilege(true);
#endif

	Logger::Instance()
#ifdef Q_OS_WIN
		.AddDebugOutputLogger()
#endif
#ifdef Q_OS_MAC
        .AddSink(std::make_shared<QDebugSink>())
#endif
		.AddFileLogger("xamp.log");		

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    SingleInstanceApplication singleApp;
    if (!singleApp.attach(QCoreApplication::arguments())) {
		return -1;
	}		

    (void) xamp::base::RNG::Instance();

	try {
		Database::Instance().open(Q_UTF8("xamp.db"));
	}
	catch (const std::exception& e) {
		XAMP_LOG_INFO("Initial database failure! {}", e.what());
		return -1;
	}

	loadAndDefaultAppConfig();

    Xamp win;
    win.show();
	return app.exec();
}
