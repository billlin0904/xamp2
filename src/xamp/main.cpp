#include "xamp.h"

#include <base/logger.h>
#include <player/audio_player.h>

#include "widget/database.h"

#include <QtWidgets/QApplication>

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

	AppSettings::settings().loadIniFile("xamp.ini");

	Xamp win;
	win.show();
	return app.exec();
}
