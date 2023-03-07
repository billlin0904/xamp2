//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>

class QWidget;
class QRect;
class QPainter;

class WheelableWidget : public QWidget {
	Q_OBJECT

public:
	static constexpr auto kWheelScrollOffset = 50000;
	static constexpr auto kScrollTime = 200;

	explicit WheelableWidget(bool touch, QWidget *parent = nullptr);

    ~WheelableWidget() override = default;

	Q_DISABLE_COPY(WheelableWidget)

    virtual void PaintBackground(QPainter* painter) = 0;

	virtual void PaintItem(QPainter* painter, int32_t index, QRect &rect) = 0;

	virtual void PaintItemMask(QPainter* painter) = 0;

	virtual int32_t ItemHeight() const = 0;

	virtual int32_t ItemCount() const = 0;

	int32_t CurrentIndex() const;

	void SetCurrentIndex(const int32_t index);

private:
	void paintEvent(QPaintEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent *event) override;

signals:
	void stopped(int32_t index);

	void ChangeTo(int32_t index);

public slots:
	void ScrollTo(int32_t index);

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

