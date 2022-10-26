#include <QMouseEvent>
#include <widget/str_utilts.h>
#include <widget/clickablelabel.h>

ClickableLabel::ClickableLabel(QWidget* parent)
	: QLabel(parent) {
	setMouseTracking(true);
	setStyleSheet(Q_TEXT("background: transparent; border: none;"));
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
