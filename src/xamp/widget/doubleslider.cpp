#include <widget/doubleslider.h>

DoubleSlider::DoubleSlider(QWidget *parent)
    : QSlider(parent) {
    QObject::connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(notifyValueChanged(int)));
}

void DoubleSlider::notifyValueChanged(int value) {
    double doubleValue = value / 10.0;
    emit doubleValueChanged(doubleValue);
}
