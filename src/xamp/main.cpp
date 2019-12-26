#include <base/logger.h>
#include <player/audio_player.h>
#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>

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
	Logger::Instance()
#ifdef _WIN32
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
