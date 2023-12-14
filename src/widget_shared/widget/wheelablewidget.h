//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <widget/widget_shared_global.h>

class QWidget;
class QRect;
class QPainter;

class WheelableWidget : public QWidget {
	Q_OBJECT
public:
	static constexpr auto kWheelScrollOffset = 50000;
	static constexpr auto kScrollTime = 200;

	explicit WheelableWidget(bool touch, QWidget *parent = nullptr);

    virtual ~WheelableWidget() override = default;

	Q_DISABLE_COPY(WheelableWidget)

    virtual void paintBackground(QPainter* painter) = 0;

	virtual void paintItem(QPainter* painter, int32_t index, QRect &rect) = 0;

	virtual void paintItemMask(QPainter* painter) = 0;

	virtual int32_t itemHeight() const = 0;

	virtual int32_t itemCount() const = 0;

	int32_t currentIndex() const;

	void setCurrentIndex(const int32_t index);

private:
	void paintEvent(QPaintEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent *event) override;

signals:
	void stopped(int32_t index);

	void changeTo(int32_t index);

public slots:
	void onScrollTo(int32_t index);

private:
	bool event(QEvent *event) override;

protected:
	bool is_scrolled_;
	bool do_signal_;
	int32_t item_;
	int32_t item_offset_; // 0-itemHeight()
	float mask_length_;
	QRect current_roll_rect_;
	QFont current_mask_font_;
	QString real_current_text_;
};

