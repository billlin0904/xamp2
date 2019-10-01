#include "xamp.h"

#include <base/logger.h>
#include <player/audio_player.h>

#include "widget/appsettings.h"
#include "widget/database.h"

#include <QtWidgets/QApplication>

static void loadAndDefaultAppConfig() {
	AppSettings::settings().loadIniFile("xamp.ini");
	AppSettings::settings().setDefaultValue(APP_SETTING_DEVICE_TYPE, "");
	AppSettings::settings().setDefaultValue(APP_SETTING_DEVICE_ID, "");
	AppSettings::settings().setDefaultValue(APP_SETTING_WIDTH, 600);
	AppSettings::settings().setDefaultValue(APP_SETTING_HEIGHT, 500);
	AppSettings::settings().setDefaultValue(APP_SETTING_VOLUME, 50);
	AppSettings::settings().setDefaultValue(APP_SETTING_NIGHT_MODE, false);
	AppSettings::settings().setDefaultValue(APP_SETTING_ORDER, PLAYER_ORDER_REPEAT_ONCE);
}

int main(int argc, char *argv[]) {
	Logger::Instance()
		.AddDebugOutputLogger()
		.AddFileLogger("xamp.log");	

	QApplication app(argc, argv);

	try {
		Database::Instance().open("xamp.db");
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
