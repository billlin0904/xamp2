#include <widget/doubleslider.h>

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent) {
    QObject::connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(NotifyValueChanged(int)));
}

void DoubleSlider::NotifyValueChanged(int value) {
	const double double_value = value / ratio_;
    emit DoubleValueChanged(double_value);
}
