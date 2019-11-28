#include <widget/qtoucheventfilter.h>

QTouchEventFilter::QTouchEventFilter(QObject* parent)
	: QObject(parent) {
}

bool QTouchEventFilter::eventFilter(QObject* p_obj, QEvent* p_event) {
	if (p_event->type() == QEvent::TouchBegin ||
		p_event->type() == QEvent::TouchUpdate ||
		p_event->type() == QEvent::TouchEnd ||
		p_event->type() == QEvent::TouchCancel) {
		p_event->ignore();
		return true;
	}
	return false;
}