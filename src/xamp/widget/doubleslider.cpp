#include <widget/doubleslider.h>

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent) {
    QObject::connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(notifyValueChanged(int)));
}

void DoubleSlider::notifyValueChanged(int value) {
    double double_value = value / base_;
    emit doubleValueChanged(double_value);
}
