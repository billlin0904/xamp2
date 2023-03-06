#include <QStyle>

#include <thememanager.h>
#include <widget/str_utilts.h>
#include <widget/seekslider.h>

SeekSlider::SeekSlider(QWidget* parent)
	: QSlider(parent) {
}

void SeekSlider::SetRange(int64_t min, int64_t max) {
	min_ = min;
	max_ = max;
	QSlider::setRange(static_cast<int>(min), static_cast<int>(max));
}

void SeekSlider::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		event->accept();
		int64_t value = 0;
		if (orientation() == Qt::Horizontal) {
			auto x = event->pos().x();
			value = ((max_ - min_) * x / width()) + min_;
		}
		else {
			auto y = event->pos().y();
			value = ((max_ - min_) * (height() - y) / height()) + min_;
		}
		setValue(value);
		emit LeftButtonValueChanged(value);
	}
	return QSlider::mousePressEvent(event);
}

void SeekSlider::enterEvent(QEvent* event) {
	qTheme.SetSliderTheme(this, true);
}

void SeekSlider::leaveEvent(QEvent* event) {
	qTheme.SetSliderTheme(this, false);
}

void SeekSlider::wheelEvent(QWheelEvent* event) {
	constexpr int kVolumeSensitivity = 30;
	const uint step = event->angleDelta().y() / kVolumeSensitivity;
	setValue(value() + step);
}