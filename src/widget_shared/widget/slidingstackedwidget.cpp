#include <QGraphicsOpacityEffect>
#include <widget/slidingstackedwidget.h>

#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QResizeEvent>
#include <QTimer>
#include <QWidget>

namespace {
constexpr int kTopLeftCornerRadius = 8;

class TopLeftCornerOverlay final : public QWidget {
public:
    explicit TopLeftCornerOverlay(QWidget* parent)
        : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        const auto color_name = parentWidget() != nullptr
            ? parentWidget()->property("topLeftCornerOutsideColor").toString()
            : QString{};
        auto fill_color = QColor(color_name);
        if (!fill_color.isValid()) {
            fill_color = palette().window().color();
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QPainterPath outside;
        outside.addRect(QRectF(rect()));

        QPainterPath rounded_corner;
        rounded_corner.addEllipse(QRectF(0, 0, kTopLeftCornerRadius * 2, kTopLeftCornerRadius * 2));

        painter.fillPath(outside.subtracted(rounded_corner), fill_color);
    }
};
} // namespace

SlidingStackedWidget::SlidingStackedWidget(QWidget* parent)
    : QStackedWidget(parent)
	, wrap_(false)
	, active_(false)
	, animation_enabled_(true)
	, direction_(Qt::Vertical)
	, speed_(300)
	, animation_type_(QEasingCurve::OutQuad)
	, current_index_(0)
	, next_index_(0)
	, previous_pos_(0, 0)
	, opacity_animation_(new QPropertyAnimation(this))
    , top_left_corner_overlay_(new TopLeftCornerOverlay(this)) {
    (void)QObject::connect(this,
        &QStackedWidget::currentChanged,
        this,
        [this] {
            updateCornerOverlay();
            QTimer::singleShot(0, this, [this] {
                updateCornerOverlay();
            });
        });
    updateCornerOverlay();
}

bool SlidingStackedWidget::event(QEvent* event) {
    if (event->type() == QEvent::DynamicPropertyChange ||
        event->type() == QEvent::StyleChange ||
        event->type() == QEvent::PaletteChange) {
        updateCornerOverlay();
    }
    return QStackedWidget::event(event);
}

void SlidingStackedWidget::resizeEvent(QResizeEvent* event) {
    QStackedWidget::resizeEvent(event);
    updateCornerOverlay();
}

void SlidingStackedWidget::updateCornerOverlay() {
    top_left_corner_overlay_->setGeometry(0, 0, kTopLeftCornerRadius, kTopLeftCornerRadius);
    top_left_corner_overlay_->raise();
    top_left_corner_overlay_->update();
}

void SlidingStackedWidget::setDirection(Qt::Orientation direction) {
    direction_ = direction;
}

void SlidingStackedWidget::setSpeed(int speed) {
    speed_ = speed;
}

void SlidingStackedWidget::setAnimation(QEasingCurve::Type animation_type) {
    animation_type_ = animation_type;
}

void SlidingStackedWidget::setWrap(bool wrap) {
    wrap_ = wrap;
}

void SlidingStackedWidget::slideInPrev() {
    auto current = currentIndex();
    if (wrap_ || current > 0) {
        slideInIndex(current - 1);
    }
}

void SlidingStackedWidget::slideInNext() {
    auto current = currentIndex();
    if (wrap_ || current < (count() - 1)) {
        slideInIndex(current + 1);
    }
}

void SlidingStackedWidget::slideInIndex(int index) {
    if (index >= count()) {
        index = index % count();
    }
    else if (index < 0) {
        index = (index + count()) % count();
    }
    slideInWidget(widget(index));
}

void SlidingStackedWidget::slideInWidget(QWidget* new_widget) {
    if (active_)
        return;

    auto current = currentIndex();
    auto next = indexOf(new_widget);

    if (current == next) {
        return;
    }

    if (!animation_enabled_) {
        setCurrentIndex(next);
        updateCornerOverlay();
        return;
    }

    active_ = true;

    auto offset_x = frameRect().width();
    auto offset_y = frameRect().height();
    widget(next)->setGeometry(frameRect());

    if (direction_ == Qt::Vertical) {
        offset_x = 0;
        offset_y = (current < next) ? -offset_y : offset_y;
    }
    else {
        offset_y = 0;
        offset_x = (current < next) ? -offset_x : offset_x;
    }

    auto next_pos = widget(next)->pos();
    auto current_pos = widget(current)->pos();
    previous_pos_ = current_pos;

    QPoint offset(offset_x, offset_y);
    widget(next)->move(next_pos - offset);
    widget(next)->show();
    widget(next)->raise();
    updateCornerOverlay();

    auto* opacity_effect = new QGraphicsOpacityEffect(new_widget);
    new_widget->setGraphicsEffect(opacity_effect);
    opacity_animation_ = new QPropertyAnimation(
        opacity_effect, "opacity");
    opacity_animation_->setDuration(speed_);
    opacity_animation_->setStartValue(0.01);
    opacity_animation_->setEndValue(1.0);
    opacity_animation_->setEasingCurve(animation_type_);

    auto* anim_group = new QParallelAnimationGroup(this);
    (void)QObject::connect(anim_group,
        &QParallelAnimationGroup::finished, 
        this, 
        &SlidingStackedWidget::animationDone);

    anim_group->addAnimation(opacity_animation_);

    for (auto index : {current, next}) {
        auto* animation = new QPropertyAnimation(
            widget(index), "pos");
        animation->setEasingCurve(animation_type_);
        animation->setDuration(speed_);
        animation->setStartValue(
            (index == current) ? current_pos : next_pos - offset);
        animation->setEndValue(
            (index == current) ? current_pos + offset : next_pos);
        anim_group->addAnimation(animation);
    }

    next_index_ = next;
    current_index_ = current;
    anim_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void SlidingStackedWidget::setAnimationEnabled(bool enabled) {
	animation_enabled_ = enabled;
}

bool SlidingStackedWidget::isAnimationEnabled() const {
    return animation_enabled_;
}

void SlidingStackedWidget::animationDone() {
    setCurrentIndex(next_index_);
    widget(current_index_)->hide();
    widget(current_index_)->move(previous_pos_);
    active_ = false;
    updateCornerOverlay();
}
