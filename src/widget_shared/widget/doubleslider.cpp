#include <QMouseEvent>
#include <widget/doubleslider.h>

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent) {
    (void)QObject::connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(NotifyValueChanged(int)));
	animation_ = new QPropertyAnimation(this, "value");
}

void DoubleSlider::NotifyValueChanged(int value) {
	const double double_value = value / ratio_;
    emit DoubleValueChanged(double_value);
}

void DoubleSlider::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		auto y = event->pos().y();
		auto value = ((maximum() - minimum()) * (height() - y) / height()) + minimum();

		event->accept();
		animation_->stop();
		animation_->setDuration(duration_);
		animation_->setEasingCurve(easing_curve_);
		animation_->setStartValue(QSlider::value());
		animation_->setEndValue(value);
		animation_->start();
	}
	return QSlider::mousePressEvent(event);
}