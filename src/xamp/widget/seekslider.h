#pragma once

#include <QSlider>
#include <QPointer>
#include <QMouseEvent>
#include <QVariantAnimation>

class SeekSlider : public QSlider {
	Q_OBJECT
public:
	SeekSlider(QWidget* parent = nullptr);

signals:
	void leftButtonValueChanged(int value);

protected:
	void mousePressEvent(QMouseEvent* event) override;
};

