//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QLabel>

class XFrame : public QFrame {
	Q_OBJECT
public:
	explicit XFrame(QWidget* parent = nullptr);

	void setContentWidget(QWidget* content);

	void setTitle(const QString& title) const;

	QWidget* contentWidget() const {
		return content_;
	}
signals:
	void closeFrame();

private:
	QLabel* title_frame_label{ nullptr };
	QWidget* content_{ nullptr };
};