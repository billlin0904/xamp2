#include <widget/stareditor.h>

#include <QDebug>
#include <cmath>

StarEditor::StarEditor(int row, QWidget* parent)
    : QWidget(parent)
    , row_(row) {
    setMouseTracking(true);
    setAutoFillBackground(true);
}

QSize StarEditor::sizeHint() const {
    return rating_.sizeHint();
}

void StarEditor::paintEvent(QPaintEvent* /* event */) {
    QPainter painter(this);
    rating_.paint(&painter, rect(), this->palette(), StarRating::Editable);
}

void StarEditor::mouseMoveEvent(QMouseEvent* event) {
    auto star = starAtPosition(event->x());

    if (star != rating_.starCount() && star != -1) {
        rating_.setStarCount(star);
        update();
    }
}

void StarEditor::mouseReleaseEvent(QMouseEvent* /* event */) {
    emit editingFinished();
}

int StarEditor::starAtPosition(int x) const {
    if (x == 0) {
        qDebug() << "StarEditor set 0 star at" << x;
        return 0;
    }
    auto star_width = double(rating_.sizeHint().width()) / double(rating_.maxStarCount());    
    auto star = (x / std::nearbyint(star_width)) + 1;
    if (star < 0 || star > rating_.maxStarCount())
        return -1;
    qDebug() << "StarEditor set " << star << " star at" << x;
    return static_cast<int32_t>(star);
}
