#include <widget/seekslider.h>

#include <QStyle>
#include <thememanager.h>
#include <widget/util/str_utilts.h>

SeekSlider::SeekSlider(QWidget* parent)
	: QSlider(parent) {
	animation_ = new QPropertyAnimation(this, "value");
}

void SeekSlider::setRange(int64_t min, int64_t max) {
	min_ = min;
	max_ = max;
	QSlider::setRange(static_cast<int>(min), static_cast<int>(max));
}

void SeekSlider::setValueAnimation(int value, bool animate) {
	target_ = value;
	if (animate) {
		animation_->stop();
		animation_->setDuration(duration_);
		animation_->setEasingCurve(easing_curve_);
		animation_->setStartValue(QSlider::value());
		animation_->setEndValue(value);
		animation_->start();
		return;
	}
	QSlider::setValue(value);
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
		setValueAnimation(value, true);
		emit leftButtonValueChanged(value);
	}
	return QSlider::mousePressEvent(event);
}

void SeekSlider::enterEvent(QEnterEvent* event) {
	qTheme.setSliderTheme(this, true);
}

void SeekSlider::leaveEvent(QEvent* event) {
	qTheme.setSliderTheme(this, false);
}

void SeekSlider::wheelEvent(QWheelEvent* event) {
	constexpr int kVolumeSensitivity = 30;
	const uint step = event->angleDelta().y() / kVolumeSensitivity;
	setValueAnimation(value() + step, true);
}