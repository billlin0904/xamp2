//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <widget/widget_shared_global.h>

class QMouseEvent;
class QEvent;
class QResizeEvent;

class XAMP_WIDGET_SHARED_API ClickableLabel final : public QLabel {
	Q_OBJECT
public:
	explicit ClickableLabel(QWidget* parent = nullptr);

	ClickableLabel(const QString& text, QWidget* parent = nullptr);

	void setText(const QString& text);

	QString text() const;

	void setElideMode(Qt::TextElideMode mode);

	Qt::TextElideMode elideMode() const;

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void changeEvent(QEvent* event) override;

private:
	void updateElidedText();

	QString text_;
	Qt::TextElideMode elide_mode_{ Qt::ElideNone };
};
