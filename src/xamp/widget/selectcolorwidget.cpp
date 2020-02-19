#include <QButtonGroup>
#include <QPushButton>
#include <QGridLayout>

#include <widget/str_utilts.h>
#include <widget/selectcolorwidget.h>

SelectColorWidget::SelectColorWidget(QWidget* parent)
    : QWidget(parent) {
    colors_.append(QColor("#121212"));
    colors_.append(QColor(228, 233, 237, 204));
    colors_.append(QColor(255, 185, 0, 204));
    colors_.append(QColor(231, 72, 86, 204));
    colors_.append(QColor(0, 120, 215, 204));
    colors_.append(QColor(0, 153, 188, 204));
    colors_.append(QColor(122, 117, 116, 204));
    colors_.append(QColor(118, 118, 118, 204));
    colors_.append(QColor(255, 140, 0, 204));
    colors_.append(QColor(232, 17, 35, 204));
    colors_.append(QColor(0, 99, 177, 204));
    colors_.append(QColor(45, 125, 154, 204));
    colors_.append(QColor(93, 90, 88, 204));
    colors_.append(QColor(76, 74, 72, 204));
    colors_.append(QColor(247, 99, 12, 204));
    colors_.append(QColor(234, 0, 94, 204));
    colors_.append(QColor(142, 140, 216, 204));
    colors_.append(QColor(0, 183, 195, 204));
    colors_.append(QColor(104, 118, 138, 204));
    colors_.append(QColor(105, 121, 126, 204));
    colors_.append(QColor(202, 80, 16, 204));
    colors_.append(QColor(195, 0, 82, 204));
    colors_.append(QColor(107, 105, 214, 204));
    colors_.append(QColor(3, 131, 135, 204));
    colors_.append(QColor(104, 118, 138, 204));
    colors_.append(QColor(105, 121, 126, 204));
    colors_.append(QColor(202, 80, 16, 204));
    colors_.append(QColor(195, 0, 82, 204));
    colors_.append(QColor(107, 105, 214, 204));
    colors_.append(QColor(3, 131, 135, 204));
    colors_.append(QColor(81, 92, 107, 204));
    colors_.append(QColor(74, 84, 89, 204));
    colors_.append(QColor(218, 59, 1, 204));
    colors_.append(QColor(227, 0, 140, 204));
    colors_.append(QColor(0, 178, 148, 204));
    colors_.append(QColor(86, 124, 115, 204));
    colors_.append(QColor(100, 124, 100, 204));
    colors_.append(QColor(239, 105, 80, 204));
    colors_.append(QColor(191, 0, 119, 204));
    colors_.append(QColor(116, 77, 169, 204));
    colors_.append(QColor(1, 133, 116, 204));
    colors_.append(QColor(72, 104, 96, 204));
    colors_.append(QColor(82, 94, 84, 204));
    colors_.append(QColor(209, 52, 56, 204));
    colors_.append(QColor(194, 57, 179, 204));
    colors_.append(QColor(177, 70, 194, 204));
    colors_.append(QColor(0, 204, 106, 204));
    colors_.append(QColor(73, 130, 5, 204));
    colors_.append(QColor(132, 117, 69, 204));
    colors_.append(QColor(255, 67, 67, 204));
    colors_.append(QColor(154, 0, 137, 204));
    colors_.append(QColor(136, 23, 152, 204));
    colors_.append(QColor(16, 137, 62, 204));
    colors_.append(QColor(16, 124, 16, 204));
    colors_.append(QColor(126, 115, 95, 204));

    group_ = new QButtonGroup(this);
    auto layout = new QGridLayout(this);

    int i = 0;
    for (auto color : colors_) {
        auto col = i % 6;
        auto row = i / 6;
        auto btn = new QPushButton(this);
        btn->setStyleSheet(backgroundColorToString(color));
        layout->addWidget(btn, row, col);
        group_->addButton(btn, i);
        ++i;
    }

    (void) QObject::connect(group_, 
        static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](auto index) {
        emit colorButtonClicked(colors_[index]);
        });
}
