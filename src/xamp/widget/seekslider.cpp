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
