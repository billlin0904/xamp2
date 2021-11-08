//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSlider>

class DoubleSlider : public QSlider {
    Q_OBJECT

public:
    DoubleSlider(QWidget *parent = nullptr);

signals:
    void doubleValueChanged(double value);

public slots:
    void notifyValueChanged(int value);

private:
    double base_{10};
};

