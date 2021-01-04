//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>

#include <widget/scrolllabel.h>

class PlayListTableView;

class PlyalistPage : public QFrame {
	Q_OBJECT
public:
	explicit PlyalistPage(QWidget *parent = nullptr);

	PlayListTableView* playlist();

	QLabel* cover();

    ScrollLabel* title();

	QLabel* format();

public slots:
    void OnThemeColorChanged(QColor theme_color, QColor color);

private:
	void initial();

	PlayListTableView* playlist_;
	QLabel* cover_;
    ScrollLabel* title_;
	QLabel* format_;
};
