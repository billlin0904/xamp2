#include <widget/clickablelabel.h>

#include <QMouseEvent>
#include <widget/str_utilts.h>

ClickableLabel::ClickableLabel(QWidget* parent)
	: QLabel(parent) {
	setMouseTracking(true);
	setStyleSheet(qTEXT("background: transparent; border: none;"));
}

void ClickableLabel::mousePressEvent(QMouseEvent*) {
	emit clicked();
}

void ClickableLabel::mouseMoveEvent(QMouseEvent* event) {
	if (rect().contains(event->pos())) {
		setCursor(Qt::PointingHandCursor);
	}
	else {
		setCursor(Qt::ArrowCursor);
	}
}
