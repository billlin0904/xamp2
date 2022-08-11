//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPen>
#include <QBrush>

class QLabel;

class ToolTips : public QFrame {
	Q_OBJECT
public:
	explicit ToolTips(const QString& text = QString(), QWidget* parent = nullptr);

public slots:
	void setText(const QString& text);

protected:
	void paintEvent(QPaintEvent*) override;

	void enterEvent(QEvent*) override;
private:
	QPen pen_;
	QBrush brush_;
	QLabel* text_label_;
};
