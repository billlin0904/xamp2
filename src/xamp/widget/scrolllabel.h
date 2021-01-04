//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <QStaticText>
#include <QTimer>
#include <QPaintEvent>
#include <QShowEvent>

#include <widget/str_utilts.h>

class ScrollLabel : public QLabel {
public:
	explicit ScrollLabel(QWidget *parent = nullptr);

	void setText(const QString &text);

	QString text() const;

public slots:
	void onTimerTimeout();

private:
	QSize sizeHint() const override;

	void paintEvent(QPaintEvent* event) override;

	void showEvent(QShowEvent* event) override;

	void hideEvent(QHideEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void updateText();

    const QLatin1String seperator = QLatin1String("   ");

	bool scrollEnabled_;
	bool waiting_;
	int leftMargin_;
	int scrollPos_;
	int singleTextWidth_;
    QStaticText static_text_;
	QString text_;
	QTimer timer_;
	QTimer wait_timer_;
	QSize wholeTextSize_;
	QImage buffer_;
	QImage alphaChannel_;
};

