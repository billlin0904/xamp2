#include <widget/clickablelabel.h>

#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <widget/util/str_util.h>

ClickableLabel::ClickableLabel(QWidget* parent)
	: ClickableLabel(QString(), parent) {
}

ClickableLabel::ClickableLabel(const QString& text, QWidget* parent)
	: QLabel(parent) {
	setMouseTracking(true);
	setStyleSheet("background: transparent; border: none;"_str);
	setText(text);
}

void ClickableLabel::setText(const QString& text) {
	text_ = text;
	updateElidedText();
}

QString ClickableLabel::text() const {
	return text_;
}

void ClickableLabel::setElideMode(Qt::TextElideMode mode) {
	if (elide_mode_ == mode) {
		return;
	}

	elide_mode_ = mode;
	updateElidedText();
}

Qt::TextElideMode ClickableLabel::elideMode() const {
	return elide_mode_;
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

void ClickableLabel::resizeEvent(QResizeEvent* event) {
	QLabel::resizeEvent(event);
	if (elide_mode_ != Qt::ElideNone) {
		updateElidedText();
	}
}

void ClickableLabel::changeEvent(QEvent* event) {
	QLabel::changeEvent(event);
	if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
		updateElidedText();
	}
}

void ClickableLabel::updateElidedText() {
	if (elide_mode_ == Qt::ElideNone) {
		QLabel::setText(text_);
		setToolTip(QString());
		return;
	}

	const auto elided_text = fontMetrics().elidedText(text_, elide_mode_, width());
	QLabel::setText(elided_text);
	setToolTip(elided_text == text_ ? QString() : text_);
}
