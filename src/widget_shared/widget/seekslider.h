//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSlider>
#include <QPointer>
#include <QMouseEvent>
#include <QVariantAnimation>
#include <QPropertyAnimation>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT SeekSlider final : public QSlider {
	Q_OBJECT
public:
    explicit SeekSlider(QWidget* parent = nullptr);

	void setRange(int64_t min, int64_t max);

	void enableAnimation(bool enable);

signals:
	void leftButtonValueChanged(int64_t value);

protected:
	void mousePressEvent(QMouseEvent* event) override;

    void enterEvent(QEnterEvent* event) override;

	void leaveEvent(QEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

private:
	void setValueAnimation(int value, bool animate);

	int target_ = 0;
	int duration_ = 100;
	int64_t min_ = 0;
	int64_t max_ = 0;
	QVariantAnimation* animation_;
	QEasingCurve easing_curve_ = QEasingCurve(QEasingCurve::Type::InOutCirc);
};

