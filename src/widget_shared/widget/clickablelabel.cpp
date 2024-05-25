#include <widget/clickablelabel.h>

#include <QMouseEvent>
#include <widget/util/str_util.h>

ClickableLabel::ClickableLabel(QWidget* parent)
	: ClickableLabel(QString(), parent) {
}

ClickableLabel::ClickableLabel(const QString& text, QWidget* parent)
	: QLabel(text, parent) {
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
