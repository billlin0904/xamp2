//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
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

	void SetRange(int64_t min, int64_t max);

signals:
	void LeftButtonValueChanged(int64_t value);

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void enterEvent(QEvent* event) override;

private:
	int64_t min_ = 0;
	int64_t max_ = 0;

};

