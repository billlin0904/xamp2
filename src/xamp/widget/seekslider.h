#pragma once

#include <QSlider>
#include <QMouseEvent>

class SeekSlider : public QSlider {
	Q_OBJECT
public:
	SeekSlider(QWidget* parent = nullptr);

signals:
	void leftButtonValueChanged(int value);

protected:
	void mousePressEvent(QMouseEvent* event) override;
};

