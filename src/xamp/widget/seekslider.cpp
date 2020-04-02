#include <widget/str_utilts.h>
#include <widget/seekslider.h>

SeekSlider::SeekSlider(QWidget* parent)
	: QSlider(parent) {
}

void SeekSlider::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		event->accept();
		auto x = event->pos().x();
		auto value = (maximum() - minimum()) * x / width() + minimum();
		emit leftButtonValueChanged(value);
	}
	return QSlider::mousePressEvent(event);
}
