#include <widget/fonticonanimation.h>
#include <QPaintEvent>
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
    ((QWidget*)parent())->update();
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

QPixmap PixmapGenerator::pixmap(QSize size) {
    QPixmap pixmap;
    if (!pixmap_cache_.isEmpty()) {
        pixmap = pixmap_cache_[animation_->currentFrame()];
        animation_->incrementFrame();
        return pixmap;
    }

    for (auto i = animation_->min(); i <= animation_->max(); ++i) {
        QImage image(size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        if (animation_) {
            animation_->transform(painter, size);
        }
        pixmap = QPixmap::fromImage(image);
        pixmap_cache_.push_back(pixmap);
        animation_->incrementFrame();
    }        
    return pixmap_cache_[animation_->currentFrame()];
}

FontIconAnimationLabel::FontIconAnimationLabel(FontIconAnimation* animation, QWidget* parent)
    : QLabel(parent)
    , generator_(animation, parent) {
}

void FontIconAnimationLabel::paintEvent(QPaintEvent* e) {
    auto rect = e->rect();

    QSize size;
    if (rect.width() > rect.height())
        size = QSize(rect.height(), rect.height());
    else
        size = QSize(rect.width(), rect.width());

    auto pixmap = generator_.pixmap(size);

    QPainter p(this);
    auto half_size = size / 2;
    auto point = rect.center() - QPoint(half_size.width(), half_size.height());
    p.drawPixmap(point, pixmap);
    p.end();
}
