#include <widget/fonticonanimation.h>

#include <QTimer>
#include <QPainter>
#include <QWidget>

FontIconAnimation::FontIconAnimation(QWidget* parent)
    : QObject(parent)
    , timer_(nullptr) {
    timer_ = new QTimer();
    (void)QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(incrementFrame()));
}

void FontIconAnimation::start() {
    timer_->start(17);
}

void FontIconAnimation::paint(QPainter& painter, const QRect& rect) {
    transform(painter, QSize(rect.width(), rect.height()));
}

void FontIconAnimation::incrementFrame() {
    if (frame_ == max_frame_)
        frame_ = min_frame_;
    else
        frame_ += 1;
}

FontIconSpinAnimation::FontIconSpinAnimation(QWidget* parent, Directions direction, int rpm)
    : FontIconAnimation(parent) {
    max_frame_ = static_cast<int>(60.0 / (rpm / 60.0));
    direction_ = direction;
}

void FontIconSpinAnimation::transform(QPainter& painter, QSize size) {
	const auto half_size = size / 2;
    auto rotation = 360.0 / max_frame_;
    if (direction_ == Directions::ANTI_CLOCKWISE) {
        rotation *= -1;
    }

    painter.translate(half_size.width(), half_size.height());
    painter.scale(0.8, 0.8);
    painter.rotate(rotation * frame_);
    painter.translate(-half_size.width(), -half_size.height());
}

PixmapGenerator::PixmapGenerator(FontIconAnimation* animation, QWidget* parent) 
    : QObject(parent)
    , animation_(animation) {
}

void PixmapGenerator::init(QSize size) {
    for (auto i = animation_->min(); i < animation_->max(); ++i) {
        pixmap(size);
    }
}

QPixmap PixmapGenerator::pixmap(QSize size) {
    if (pixmap_cache_.contains(animation_->currentFrame())) {
        return pixmap_cache_[animation_->currentFrame()];
    }

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    if (animation_) {
        animation_->transform(painter, size);
    }

    auto pixamp = QPixmap::fromImage(image);
    pixmap_cache_.insert(std::make_pair(animation_->currentFrame(), pixamp));
    return pixamp;
}
