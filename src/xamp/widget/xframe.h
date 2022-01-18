//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

class XFrame : public QFrame {
	Q_OBJECT
public:
	explicit XFrame(QWidget* parent = nullptr);

	void setContentWidget(QWidget* content);

signals:
	void closeFrame();

private:
	QWidget* content_widget_{ nullptr };
};