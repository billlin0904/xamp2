//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <QStaticText>
#include <QTimer>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_API ScrollLabel final : public QLabel {
public:
	explicit ScrollLabel(QWidget *parent = nullptr);

	void setText(const QString &text);

	QString text() const;

	void setElideMode(Qt::TextElideMode mode);

	Qt::TextElideMode elideMode() const;

public slots:
	void onTimerTimeout();

private:
	QSize sizeHint() const override;

	void paintEvent(QPaintEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void changeEvent(QEvent* event) override;

	void updateText();

    const QLatin1String seperator_ = QLatin1String("   ");

	bool scroll_enabled_;
	bool waiting_;
	Qt::TextElideMode elide_mode_{ Qt::ElideNone };
	int left_margin_;
	int scroll_pos_;
	int single_text_width_;
    QStaticText static_text_;
	QString text_;
	QTimer timer_;
	QTimer wait_timer_;
	QSize whole_text_size_;
	QImage buffer_;
	QImage alpha_channel_;
};

