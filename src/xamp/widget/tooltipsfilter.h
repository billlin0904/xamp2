//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QObject>
#include <QTimer>

class ToolTipsFilter : public QObject {
	Q_OBJECT
public:
	explicit ToolTipsFilter(QObject* parent = nullptr);

	bool eventFilter(QObject* obj, QEvent* event) override;
private:
	void showTooltip(QWidget* tooltip);
	QTimer timer_;
	QWidget* parent_;
	QWidget* tooltip_;
};
