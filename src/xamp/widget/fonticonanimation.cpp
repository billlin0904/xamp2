#include <QPainter>
#include <QTimer>
#include <QWidget>
#include <widget/fonticonanimation.h>

FontIconAnimation::FontIconAnimation(QWidget* parent, int interval, int step)
    : parent_(parent)
    , timer_(nullptr)
    , interval_(interval)
    , step_(step)
    , angle_(0.0f) {
}

void FontIconAnimation::setup(QPainter& painter, const QRect& rect) {
    if (!timer_) {
        timer_ = new QTimer();
        QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(update()));
        timer_->start(interval_);
    } else {
	    const float x_center = rect.width() * 0.5f;
        const float y_center = rect.height() * 0.5f;
        painter.translate(x_center, y_center);
        painter.rotate(angle_);
        painter.translate(-x_center, -y_center);
    }
}

void FontIconAnimation::update() {
    angle_ += step_;
    angle_ = std::fmod(angle_, 360);
    if (parent_ != nullptr) {
        parent_->update();
    }
}