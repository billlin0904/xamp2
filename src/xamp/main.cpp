#include "xamp.h"

#include <iostream>

#include "widget/database.h"

#include <QDebug>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	try {
		Database::Instance().open("xamp.db");
	} catch (const std::exception& e) {
		return -1;
	}

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QApplication app(argc, argv);	
	Xamp win;
	win.show();
	return app.exec();
}
