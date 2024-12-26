//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QPoint>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT SlidingStackedWidget : public QStackedWidget {
    Q_OBJECT

public:
    explicit SlidingStackedWidget(QWidget* parent = nullptr);

    void setDirection(Qt::Orientation direction);
    void setSpeed(int speed);
    void setAnimation(QEasingCurve::Type animation_type);
    void setWrap(bool wrap);

    void slideInWidget(QWidget* new_widget);

public slots:
    void slideInPrev();
    void slideInNext();

private:
    void slideInIndex(int index);
    void animationDone();

    bool wrap_;
    bool active_;
    Qt::Orientation direction_;
    int speed_;
    QEasingCurve::Type animation_type_;
    int current_index_;
    int next_index_;
    QPoint previous_pos_;
    QPropertyAnimation* opacity_animation_;
};

