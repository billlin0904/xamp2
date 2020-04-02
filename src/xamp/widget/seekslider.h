//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSlider>
#include <QPointer>
#include <QMouseEvent>
#include <QVariantAnimation>

class SeekSlider : public QSlider {
	Q_OBJECT
public:
    explicit SeekSlider(QWidget* parent = nullptr);

signals:
	void leftButtonValueChanged(int value);

protected:
	void mousePressEvent(QMouseEvent* event) override;
};

