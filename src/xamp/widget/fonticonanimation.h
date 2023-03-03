//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

class QWidget;
class QPainter;
class QTimer;

class FontIconAnimation : public QObject {
	Q_OBJECT
public:
	explicit FontIconAnimation(QWidget* parent, int interval = 10, int step = 1);

	void setup(QPainter& painter, const QRect& rect);

public slots:
	void update();

private:
	QWidget* parent_;
	QTimer* timer_;
	int interval_;
	int step_;
	float angle_;
};

