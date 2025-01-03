#include <widget/flowlayout.h>
#include <QWidget>
#include <QVariant>

FlowLayout::FlowLayout(QWidget* parent, bool needAni, bool isTight)
    : QLayout(parent)
	, need_animation_(needAni)
	, is_tight_(isTight)
	, vertical_spacing_(10)
	, horizontal_spacing_(10)
	, animation_group_(new QParallelAnimationGroup(this)) {
}

FlowLayout::~FlowLayout() {
    removeAllItems();
}

QSize FlowLayout::sizeHint() const {
    return minimumSize();
}

void FlowLayout::addItem(QLayoutItem* item) {
    items_.append(item);
}

QLayoutItem* FlowLayout::itemAt(int index) const {
    if (index >= 0 && index < items_.count()) {
        return items_[index];
    }

    return nullptr;
}

QLayoutItem* FlowLayout::takeAt(int index) {
    if (index >= 0 && index < items_.count()) {
        QLayoutItem* item = items_[index];
        QPropertyAnimation* ani = item->widget()->property("flowAni").value<QPropertyAnimation*>();
        if (ani) {
            animations_.removeAll(ani);
            animation_group_->removeAnimation(ani);
            ani->deleteLater();
        }

        return items_.takeAt(index);
    }

    return nullptr;
}

int FlowLayout::count() const {
    return items_.count();
}

QSize FlowLayout::minimumSize() const {
    QSize size;

    for (auto item : items_) {
        size = size.expandedTo(item->minimumSize());
    }

    QMargins m = contentsMargins();
    size += QSize(m.left() + m.right(), m.top() + m.bottom());
    return size;
}

Qt::Orientations FlowLayout::expandingDirections() const {
    return Qt::Orientation(0);
}

void FlowLayout::setGeometry(const QRect& rect) {
    QLayout::setGeometry(rect);
    doLayout(rect, true);
}

bool FlowLayout::hasHeightForWidth() const {
    return true;
}

int FlowLayout::heightForWidth(int width) const {
    return doLayout(QRect(0, 0, width, 0), false);
}

//void FlowLayout::addWidget(QWidget* w) {
//    QLayout::addWidget(w);
//
//    if (!need_animation_) {
//        return;
//    }
//
//    QPropertyAnimation* ani = new QPropertyAnimation(w, "geometry");
//    ani->setEndValue(QRect(QPoint(0, 0), w->size()));
//    ani->setDuration(300);
//    w->setProperty("flowAni", QVariant::fromValue<QPropertyAnimation*>(ani));
//    animations_.append(ani);
//    animation_group_->addAnimation(ani);
//}

void FlowLayout::setAnimation(int duration /*msec*/, QEasingCurve ease) {
    if (!need_animation_) {
        return;
    }

    for (auto a : animations_) {
        a->setDuration(duration);
        a->setEasingCurve(ease);
    }
}

void FlowLayout::removeAllItems() {
    QLayoutItem* item;
    while ((item = takeAt(0)))
        delete item;
}

void FlowLayout::takeAllWidgets() {
    QLayoutItem* item;
    while ((item = takeAt(0))) {
        QWidget* w = item->widget();
        if (w) {
            w->deleteLater();
        }
        delete item;
    }
}

int FlowLayout::doLayout(const QRect& rect, bool move) const {
    QMargins margin = contentsMargins();
    int x = rect.x() + margin.left();
    int y = rect.y() + margin.top();
    int rowHeight = 0;
    int spaceX = horizontal_spacing_;
    int spaceY = vertical_spacing_;

    for (int i = 0; i < items_.count(); ++i) {
        QLayoutItem* item = items_.at(i);

        if (item->widget() && !item->widget()->isVisible() && is_tight_) {
            continue;
        }

        int nextX = x + item->sizeHint().width() + spaceX;
        if (nextX - spaceX > rect.right() && rowHeight > 0) {
            x = rect.x() + margin.left();
            y = y + rowHeight + spaceY;
            nextX = x + item->sizeHint().width() + spaceX;
            rowHeight = 0;
        }

        if (move) {
            if (!need_animation_) {
                item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            }
            else {
                animations_[i]->stop();
                animations_[i]->setEndValue(QRect(QPoint(x, y), item->sizeHint()));
            }
        }

        x = nextX;
        rowHeight = qMax(rowHeight, item->sizeHint().height());
    }

    if (need_animation_) {
        animation_group_->stop();
        animation_group_->start();
    }

    return y + rowHeight - rect.y();
}

int FlowLayout::horizontalSpacing() const {
    return horizontal_spacing_;
}

void FlowLayout::setHorizontalSpacing(int horizontalSpacing) {
    horizontal_spacing_ = horizontalSpacing;
}

int FlowLayout::verticalSpacing() const {
    return vertical_spacing_;
}

void FlowLayout::setVerticalSpacing(int verticalSpacing) {
    vertical_spacing_ = verticalSpacing;
}
