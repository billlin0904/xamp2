#include <QGraphicsOpacityEffect>
#include <widget/slidingstackedwidget.h>

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QWidget>

SlidingStackedWidget::SlidingStackedWidget(QWidget* parent)
    : QStackedWidget(parent)
	, wrap_(false)
	, active_(false)
	, direction_(Qt::Vertical)
	, speed_(150)
	, animation_type_(QEasingCurve::OutQuad)
	, current_index_(0)
	, next_index_(0)
	, previous_pos_(0, 0)
	, opacity_animation_(new QPropertyAnimation(this)) {
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

    active_ = true;

    auto current = currentIndex();
    auto next = indexOf(new_widget);

    if (current == next) {
        active_ = false;
        return;
    }

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

    auto* opacity_effect = new QGraphicsOpacityEffect(new_widget);
    new_widget->setGraphicsEffect(opacity_effect);
    opacity_animation_ = new QPropertyAnimation(opacity_effect, "opacity");
    opacity_animation_->setDuration(speed_);
    opacity_animation_->setStartValue(0.01);
    opacity_animation_->setEndValue(1.0);
    opacity_animation_->setEasingCurve(animation_type_);

    auto* anim_group = new QParallelAnimationGroup(this);
    (void)QObject::connect(anim_group, &QParallelAnimationGroup::finished, this, &SlidingStackedWidget::animationDone);

    anim_group->addAnimation(opacity_animation_);

    for (auto index : {current, next}) {
        auto* animation = new QPropertyAnimation(widget(index), "pos");
        animation->setEasingCurve(animation_type_);
        animation->setDuration(speed_);
        animation->setStartValue((index == current) ? current_pos : next_pos - offset);
        animation->setEndValue((index == current) ? current_pos + offset : next_pos);
        anim_group->addAnimation(animation);
    }

    next_index_ = next;
    current_index_ = current;
    anim_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void SlidingStackedWidget::animationDone() {
    setCurrentIndex(next_index_);
    widget(current_index_)->hide();
    widget(current_index_)->move(previous_pos_);
    active_ = false;
}
