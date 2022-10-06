#include <QToolTip>

#include "str_utilts.h"
#include <widget/tooltips.h>
#include <widget/tooltipsfilter.h>
#include <widget/seekslider.h>

SeekSlider::SeekSlider(QWidget* parent)
	: QSlider(parent) {
}

void SeekSlider::setRange(int64_t min, int64_t max) {
	min_ = min;
	max_ = max;
	QSlider::setRange(static_cast<int>(min), static_cast<int>(max));
}

void SeekSlider::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		event->accept();
		auto x = event->pos().x();
		auto value = (max_ - min_) * x / width() + min_;
		emit leftButtonValueChanged(value);
	}
	return QSlider::mousePressEvent(event);
}

void SeekSlider::enterEvent(QEvent* event) {
	auto x = mapFromGlobal(QCursor::pos()).x();
	auto value = (max_ - min_) * x / width() + min_;
	QToolTip::showText(QCursor::pos(), msToString(static_cast<double>(value) / 1000.0));
}
