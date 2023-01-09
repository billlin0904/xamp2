//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSlider>

class DoubleSlider : public QSlider {
    Q_OBJECT

public:
    explicit DoubleSlider(QWidget *parent = nullptr);

    double ratio() const {
        return ratio_;
    }

signals:
    void doubleValueChanged(double value);

public slots:
    void notifyValueChanged(int value);

private:
    double ratio_{10};
};

