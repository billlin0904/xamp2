#include <widget/starrating.h>

#include <cmath>

constexpr auto kPaintingScaleFactor = 15;

StarRating::StarRating(int starCount, int maxStarCount) {
    rating_count_ = starCount;
    max_rating_count_ = maxStarCount;

    polygon_ << QPointF(1.0, 0.5);
    for (int i = 1; i < 5; ++i)
        polygon_ << QPointF(0.5 + 0.5 * cos(0.8 * i * 3.14),
            0.5 + 0.5 * sin(0.8 * i * 3.14));

    diamond_ << QPointF(0.4, 0.5) << QPointF(0.5, 0.4)
        << QPointF(0.6, 0.5) << QPointF(0.5, 0.6)
        << QPointF(0.4, 0.5);
}

QSize StarRating::sizeHint() const {
    return kPaintingScaleFactor * QSize(max_rating_count_, 1);
}

void StarRating::paint(QPainter* painter, const QRect& rect, const QPalette&, EditMode mode) const {
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    if (mode == Editable) {
        painter->setBrush(Qt::white);
    }
    else {
        painter->setBrush(Qt::yellow);
    }

    int yOffset = (rect.height() - kPaintingScaleFactor) / 2;
    painter->translate(rect.x(), rect.y() + yOffset);
    painter->scale(kPaintingScaleFactor, kPaintingScaleFactor);

    for (int i = 0; i < max_rating_count_; ++i) {
        if (i < rating_count_) {            
            painter->drawPolygon(polygon_, Qt::WindingFill);
        }
        else if (mode == Editable) {
            painter->drawPolygon(diamond_, Qt::WindingFill);
        }
        painter->translate(1.0, 0.0);
    }

    painter->restore();
}
