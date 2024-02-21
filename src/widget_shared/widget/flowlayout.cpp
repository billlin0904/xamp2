#include <widget/flowlayout.h>

FlowLayout::FlowLayout(QWidget* parent)
    : QLayout(parent)
	, vertical_spacing_(10)
	, horizontal_spacing_(10) {
}

FlowLayout::~FlowLayout() {
    QLayoutItem* item;
    while ((item = FlowLayout::takeAt(0)))
        delete item;
}

void FlowLayout::addItem(QLayoutItem* item) {
    items_.append(item);
}

int FlowLayout::count() const {
    return items_.size();
}

QLayoutItem* FlowLayout::itemAt(int index) const {
    return items_.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index) {
    if (index >= 0 && index < items_.size())
        return items_.takeAt(index);
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const {
    return static_cast<Qt::Orientation>(0);
}

bool FlowLayout::hasHeightForWidth() const {
    return true;
}

int FlowLayout::heightForWidth(int width) const {
    return doLayout(QRect(0, 0, width, 0), false);
}

void FlowLayout::setGeometry(const QRect& rect) {
    QLayout::setGeometry(rect);
    doLayout(rect, true);
}

QSize FlowLayout::sizeHint() const {
    return minimumSize();
}

QSize FlowLayout::minimumSize() const {
    QSize size;

    for (const auto& item : items_)
        size = size.expandedTo(item->minimumSize());

    const QMargins m = contentsMargins();
    size += QSize(m.left() + m.right(), m.top() + m.bottom());

    return size;
}

void FlowLayout::setVerticalSpacing(int spacing) {
    vertical_spacing_ = spacing;
}

int FlowLayout::verticalSpacing() const {
    return vertical_spacing_;
}

void FlowLayout::setHorizontalSpacing(int spacing) {
    horizontal_spacing_ = spacing;
}

int FlowLayout::horizontalSpacing() const {
    return horizontal_spacing_;
}

int FlowLayout::doLayout(const QRect& rect, bool move) const {
	const int margin = contentsMargins().left();
    int x = rect.x() + margin;
    int y = rect.y() + margin;
    int row_height = 0;
	const int space_x = horizontalSpacing();
	const int space_y = verticalSpacing();

    for (QLayoutItem* item : items_) {
        int next_x = x + item->sizeHint().width() + space_x;

        if (next_x - space_x > rect.right() && row_height > 0) {
            x = rect.x() + margin;
            y = y + row_height + space_y;
            next_x = x + item->sizeHint().width() + space_x;
            row_height = 0;
        }

        if (move) {
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
        }

        x = next_x;
        row_height = std::max(row_height, item->sizeHint().height());
    }

    return y + row_height - rect.y();
}
