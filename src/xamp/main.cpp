#include "xamp.h"

#include <base/logger.h>

#include "widget/database.h"

#include <QDebug>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	Logger::Instance()
		.AddDebugOutputLogger()
		.AddFileLogger("xamp.log");	

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QApplication app(argc, argv);

	try {
		Database::Instance().open("xamp.db");
	}
	catch (const std::exception& e) {
		XAMP_LOG_INFO("Initial database failure! {}", e.what());
		return -1;
	}

	Xamp win;
	win.show();
	return app.exec();
}
