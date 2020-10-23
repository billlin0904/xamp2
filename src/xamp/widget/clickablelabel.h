//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <QMouseEvent>

class ClickableLabel final : public QLabel {
	Q_OBJECT
public:
	explicit ClickableLabel(QWidget* parent = nullptr);

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;
};
