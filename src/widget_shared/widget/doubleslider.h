//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPropertyAnimation>
#include <QSlider>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT DoubleSlider : public QSlider {
    Q_OBJECT

public:
    explicit DoubleSlider(QWidget *parent = nullptr);

    double ratio() const {
        return ratio_;
    }

signals:
    void DoubleValueChanged(double value);

public slots:
    void NotifyValueChanged(int value);

private:
    void mousePressEvent(QMouseEvent* event) override;

    int target_{0};
    int duration_{100};
    double ratio_{10};
    QVariantAnimation* animation_;
    QEasingCurve easing_curve_ = QEasingCurve(QEasingCurve::Type::InOutCirc);
};

