#include <QPainter>
#include <QPaintEvent>
#include <qmath.h>

#include <widget/spectrograph.h>

Spectrograph::Spectrograph(QWidget* parent)
    : QFrame(parent) {
}

void Spectrograph::setFrequency(double low_freq, double high_freq, double frequency) {
}

void Spectrograph::paintEvent(QPaintEvent*) {
	QPainter painter(this);
}

void Spectrograph::receiveMagnitude(const std::vector<float>& mag) {
}

void Spectrograph::reset() {
}

void Spectrograph::start() {
}

void Spectrograph::stop() {
}
