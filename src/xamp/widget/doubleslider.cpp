#include <widget/doubleslider.h>

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent) {
    QObject::connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(notifyValueChanged(int)));
}

void DoubleSlider::NotifyValueChanged(int value) {
    double double_value = value / ratio_;
    emit DoubleValueChanged(double_value);
}
