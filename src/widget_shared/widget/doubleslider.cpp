#include <QMouseEvent>
#include <widget/doubleslider.h>

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent) {
    (void)QObject::connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(onNotifyValueChanged(int)));
	animation_ = new QPropertyAnimation(this, "value");
}

void DoubleSlider::onNotifyValueChanged(int value) {
	const double double_value = value / ratio_;
    emit doubleValueChanged(double_value);
}

void DoubleSlider::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		const auto y = event->pos().y();
		const auto value = ((maximum() - minimum()) * (height() - y) / height()) + minimum();

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