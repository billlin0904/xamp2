//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <widget/widget_shared_global.h>

class QMouseEvent;

class XAMP_WIDGET_SHARED_EXPORT ClickableLabel final : public QLabel {
	Q_OBJECT
public:
	explicit ClickableLabel(QWidget* parent = nullptr);

	ClickableLabel(const QString& text, QWidget* parent = nullptr);

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;
};
