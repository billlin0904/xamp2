#include <widget/fonticonanimation.h>

#include <QTimer>
#include <QPainter>
#include <QWidget>

FontIconAnimation::FontIconAnimation(QWidget* parent)
    : QObject(parent)
    , timer_(nullptr) {
    timer_ = new QTimer();
    (void)QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(frame()));
    timer_->start(17);
}

void FontIconAnimation::paint(QPainter& painter, const QRect& rect) {
    transform(painter, QSize(rect.width(), rect.height()));
}

void FontIconAnimation::frame() {
    if (frame_ == max_frame_)
        frame_ = min_frame_;
    else
        frame_ += 1;
}

FontIconSpinAnimation::FontIconSpinAnimation(QWidget* parent, Directions direction, int rpm)
    : FontIconAnimation(parent) {
    max_frame_ = int(60.0 / (rpm / 60.0));
    direction_ = direction;
}

void FontIconSpinAnimation::transform(QPainter& painter, QSize size) {
    auto halfSize = size / 2;
    auto rotation = 360.0 / max_frame_;
    if (direction_ == Directions::ANTI_CLOCKWISE) {
        rotation *= -1;
    }

    painter.translate(halfSize.width(), halfSize.height());
    painter.scale(0.8, 0.8);
    painter.rotate(rotation * frame_);
    painter.translate(-halfSize.width(), -halfSize.height());
}

PixmapGenerator::PixmapGenerator(FontIconAnimation* animation, QWidget* parent) 
    : QObject(parent)
    , animation_(animation) {
}

QPixmap PixmapGenerator::pixmap(QSize size) {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    if (animation_) {
        animation_->transform(painter, size);
    }
}
