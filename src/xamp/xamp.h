#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_xamp.h"

class Xamp : public QMainWindow {
	Q_OBJECT

public:
	Xamp(QWidget *parent = Q_NULLPTR);

private:
	Ui::XampWindow ui;
};
