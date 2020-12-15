#include <QButtonGroup>
#include <QPushButton>
#include <QGridLayout>

#include "thememanager.h"

#include <widget/str_utilts.h>
#include <widget/selectcolorwidget.h>

static const QList<QColor> & GetColors() {
    static QList<QColor> colors;
    if (colors.size() > 0) {
        return colors;
    }
    colors.append(QColor(255, 185, 0, 150));
    colors.append(QColor(231, 72, 86, 150));
    colors.append(QColor(0, 120, 215, 150));
    colors.append(QColor(0, 153, 188, 150));
    colors.append(QColor(122, 117, 116, 150));
    colors.append(QColor(118, 118, 118, 150));
    colors.append(QColor(255, 140, 0, 150));
    colors.append(QColor(232, 17, 35, 150));
    colors.append(QColor(0, 99, 177, 150));
    colors.append(QColor(45, 125, 154, 150));
    colors.append(QColor(93, 90, 88, 150));
    colors.append(QColor(76, 74, 72, 150));
    colors.append(QColor(247, 99, 12, 150));
    colors.append(QColor(234, 0, 94, 150));
    colors.append(QColor(142, 140, 216, 150));
    colors.append(QColor(0, 183, 195, 150));
    colors.append(QColor(104, 118, 138, 150));
    colors.append(QColor(105, 121, 126, 150));
    colors.append(QColor(202, 80, 16, 150));
    colors.append(QColor(195, 0, 82, 150));
    colors.append(QColor(107, 105, 214, 150));
    colors.append(QColor(3, 131, 135, 150));
    colors.append(QColor(104, 118, 138, 150));
    colors.append(QColor(105, 121, 126, 150));
    colors.append(QColor(202, 80, 16, 150));
    colors.append(QColor(195, 0, 82, 150));
    colors.append(QColor(107, 105, 214, 150));
    colors.append(QColor(3, 131, 135, 150));
    colors.append(QColor(81, 92, 107, 150));
    colors.append(QColor(74, 84, 89, 150));
    colors.append(QColor(218, 59, 1, 150));
    colors.append(QColor(227, 0, 140, 150));
    colors.append(QColor(0, 178, 148, 150));
    colors.append(QColor(86, 124, 115, 150));
    colors.append(QColor(100, 124, 100, 150));
    colors.append(QColor(239, 105, 80, 150));
    colors.append(QColor(191, 0, 119, 150));
    colors.append(QColor(116, 77, 169, 150));
    colors.append(QColor(1, 133, 116, 150));
    colors.append(QColor(72, 104, 96, 150));
    colors.append(QColor(82, 94, 84, 150));
    colors.append(QColor(209, 52, 56, 150));
    colors.append(QColor(194, 57, 179, 150));
    colors.append(QColor(177, 70, 194, 150));
    colors.append(QColor(0, 1, 106, 150));
    colors.append(QColor(73, 130, 5, 150));
    colors.append(QColor(132, 117, 69, 150));
    colors.append(QColor(255, 67, 67, 150));
    colors.append(QColor(154, 0, 137, 150));
    colors.append(QColor(136, 23, 152, 150));
    colors.append(QColor(16, 137, 62, 150));
    colors.append(QColor(16, 124, 16, 150));
    colors.append(QColor(126, 115, 95, 150));
    colors.append(QColor(18, 18, 18, 150));  
    return colors;
}

SelectColorWidget::SelectColorWidget(QWidget* parent)
    : QFrame(parent) {
    auto group = new QButtonGroup(this);
    auto layout = new QGridLayout(this);

    int i = 0;
    for (auto color : GetColors()) {
        auto col = i % 6;
        auto row = i / 6;
        auto btn = new QPushButton(this);
        auto objectName = QString(Q_UTF8("color%1Button")).arg(color.rgba());
        btn->setObjectName(objectName);
        btn->setStyleSheet(QString(Q_UTF8(R"(QPushButton#%1 { background-color:%2; border: none; })"))
            .arg(objectName, colorToString(color)));
        layout->addWidget(btn, row, col);
        group->addButton(btn, i);
        ++i;
    }

    (void) QObject::connect(group,
        static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](auto index) {
        emit colorButtonClicked(GetColors()[index]);
        });

    setStyleSheet(Q_UTF8("background-color: rgb(18, 18, 18);"));
}
