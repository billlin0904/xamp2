#pragma once
//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class PlayListTableView;

class PlyalistPage : public QFrame {
	Q_OBJECT
public:
	PlyalistPage(QWidget *parent = nullptr);

	PlayListTableView* playlist();

	QLabel* cover();

	QLabel* title();

	QLabel* format();
private:
	void initial();

	PlayListTableView* playlist_;
	QLabel* cover_;
	QLabel* title_;
	QLabel* format_;
};
