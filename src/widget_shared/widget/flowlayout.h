//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLayout>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

class FlowLayout : public QLayout {
	Q_OBJECT
public:
	FlowLayout(QWidget* parent = nullptr, bool needAni = false, bool isTight = false);

	~FlowLayout() override;

public:
    QSize sizeHint() const override;
    void addItem(QLayoutItem*) override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    int count() const override;

    QSize minimumSize() const override;
    Qt::Orientations expandingDirections() const override;
    void setGeometry(const QRect&) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;

    //void addWidget(QWidget* w);
    void setAnimation(int duration, QEasingCurve ease = QEasingCurve::Linear);
    void removeAllItems();
    void takeAllWidgets();

    int verticalSpacing() const;
    void setVerticalSpacing(int verticalSpacing);

    int horizontalSpacing() const;
    void setHorizontalSpacing(int horizontalSpacing);

protected:
    int doLayout(const QRect& rect, bool move) const;

private:
    bool need_animation_;
    bool is_tight_;
    int vertical_spacing_;
    int horizontal_spacing_;
    QList<QLayoutItem*> items_;
    QList<QPropertyAnimation*> animations_;
    QParallelAnimationGroup* animation_group_;
};
