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

#include <QMessageBox>
#include <QtWidgets/QApplication>

#include "thememanager.h"
#include "singleinstanceapplication.h"
#include "xamp.h"

static void loadAndDefaultAppConfig() {
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

int main(int argc, char *argv[]) {
	Logger::Instance()
#ifdef Q_OS_WIN
		.AddDebugOutputLogger()
#endif
#ifdef Q_OS_MAC
		.AddSink(std::make_shared<QDebugSink>())
#endif
		.AddFileLogger("xamp.log");

    std::vector<ModuleHandle> preload_modules;

#ifdef Q_OS_WIN
	try {		
		// 如果沒有預先載入拖入檔案會爆音.
		preload_modules.emplace_back(LoadDll("psapi.dll"));
		preload_modules.emplace_back(LoadDll("comctl32.dll"));
		preload_modules.emplace_back(LoadDll("WindowsCodecs.dll"));
		// 為了效率考量.
		preload_modules.emplace_back(LoadDll("AUDIOKSE.dll"));
		preload_modules.emplace_back(LoadDll("avrt.dll"));
		AudioPlayer::LoadLib();
	}
	catch (const Exception & e) {
		QMessageBox::critical(nullptr, Q_UTF8("Load dll failure!"), QString::fromStdString(e.GetErrorMessage()));
		return -1;
	}

	VmMemLock::EnableLockMemPrivilege(true);
#endif

#ifdef Q_OS_MAC
    try {
        preload_modules.emplace_back(LoadDll("libchromaprint.dylib"));
        preload_modules.emplace_back(LoadDll("libbass.dylib"));
    } catch (const Exception & e) {
        QMessageBox::critical(nullptr, Q_UTF8("Load dll failure!"), QString::fromStdString(e.GetErrorMessage()));
        return -1;
    }
#endif	

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    SingleInstanceApplication singleApp;
    if (!singleApp.attach(QCoreApplication::arguments())) {
		return -1;
	}		

    (void) xamp::base::RNG::Instance();
	(void) PixmapCache::instance();

	try {
		Database::instance().open(Q_UTF8("xamp.db"));
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
