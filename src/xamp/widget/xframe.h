//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QLabel>

class XFrame : public QFrame {
	Q_OBJECT
public:
	explicit XFrame(QWidget* parent = nullptr);

	void SetContentWidget(QWidget* content);

	void SetTitle(const QString& title) const;

	QWidget* ContentWidget() const {
		return content_;
	}
signals:
	void CloseFrame();

private:
	QLabel* title_frame_label{ nullptr };
	QWidget* content_{ nullptr };
};