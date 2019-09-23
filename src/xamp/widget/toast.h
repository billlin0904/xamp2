//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QLabel>
#include <QWidget>

#include "ui_toast.h"

class Toast : public QWidget {
	Q_OBJECT
public:
	Toast(QWidget* parent = nullptr);

	~Toast();

	void setText(const QString& text);

	void showAnimation(int timeout = 2000);

	void HideAnimation();

	static void showTip(const QString& text, QWidget* parent = nullptr);

protected:
	virtual void paintEvent(QPaintEvent* event);

	Ui::Toast ui;
};