#include <widget/stareditor.h>

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
    auto star = (x / (rating_.sizeHint().width()
        / rating_.maxStarCount())) + 1;
    if (star <= 0 || star > rating_.maxStarCount())
        return -1;
    return star;
}