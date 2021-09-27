//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPushButton>

class SwitchButton : public QPushButton {
    Q_OBJECT
public:
    explicit SwitchButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *ev) override;

    void nextCheckState() override;
private:
    bool checked_{false};
    qreal progress_{0};
};
