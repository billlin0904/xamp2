#include "xamp.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	Xamp w;
	w.show();
	return a.exec();
}
