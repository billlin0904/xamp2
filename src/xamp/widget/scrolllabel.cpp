#include <QPainter>
#include <widget/scrolllabel.h>

ScrollLabel::ScrollLabel(QWidget* parent) 
	: QLabel(parent) {
	static_text_ = new QStaticText();
	static_text_->setTextFormat(Qt::PlainText);
	timer_.setInterval(16);
	timer_.setTimerType(Qt::PreciseTimer);
	(void)QObject::connect(&timer_, &QTimer::timeout, this, &ScrollLabel::onTimerTimeout);
	wait_timer_.setInterval(5000);
	(void)QObject::connect(&wait_timer_, &QTimer::timeout, this, &ScrollLabel::onTimerTimeout);
	leftMargin_ = height() / 3;
	scrollPos_ = 0;
	scrollEnabled_ = false;
	waiting_ = true;
	updateText();
}

void ScrollLabel::setText(const QString& text) {
	text_ = text;
	updateText();
	update();
	updateGeometry();
}

QString ScrollLabel::text() const {
	return text_;
}

void ScrollLabel::updateText() {
	timer_.stop();

	singleTextWidth_ = fontMetrics().width(text_);
	scrollEnabled_ = singleTextWidth_ > (width() - leftMargin_);

	if (scrollEnabled_) {
		static_text_->setText(text_ + seperator);
		if (window()->windowState() & Qt::WindowMinimized) {
			scrollPos_ = 0;
			wait_timer_.start();
			waiting_ = true;
		}
	}
	else {
		static_text_->setText(text_);
	}

	static_text_->prepare(QTransform(), font());
	wholeTextSize_ = QSize(fontMetrics().width(static_text_->text()), fontMetrics().height());
}

void ScrollLabel::onTimerTimeout() {
	scrollPos_ = (scrollPos_ + 1) % wholeTextSize_.width();

	if (waiting_) {
		waiting_ = false;
		timer_.start();
		wait_timer_.stop();
	}

	if (scrollPos_ == 0) {
		waiting_ = true;
		timer_.stop();
		wait_timer_.start();
	}
}

QSize ScrollLabel::sizeHint() const {
	return QSize(std::min(wholeTextSize_.width() + leftMargin_, maximumWidth()), fontMetrics().height());
}

void ScrollLabel::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	if (scrollEnabled_) {
		buffer_.fill(qRgba(0, 0, 0, 0));
		QPainter pb(&buffer_);
		pb.setPen(painter.pen());
		pb.setFont(painter.font());

		auto x = std::min(-scrollPos_, 0) + leftMargin_;
		while (x < width()) {
			pb.drawStaticText(QPointF(x, (height() - wholeTextSize_.height()) / 2), *static_text_);
			x += wholeTextSize_.width();
		}
		pb.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		pb.setClipRect(width() - 15, 0, 15, height());
		pb.drawImage(0, 0, alphaChannel_);
		pb.setClipRect(0, 0, 15, height());
		pb.drawImage(0, 0, alphaChannel_);
		painter.drawImage(0, 0, buffer_);
	}
	else {
		painter.drawStaticText(QPointF(leftMargin_, (height() - wholeTextSize_.height()) / 2), 	*static_text_);
	}
}

void ScrollLabel::resizeEvent(QResizeEvent* event) {
	alphaChannel_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
	buffer_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
	alphaChannel_.fill(qRgba(0, 0, 0, 0));
	buffer_.fill(qRgba(0, 0, 0, 0));

	if (width() > 64) {
		QLinearGradient grad(QPointF(0, 0), QPointF(16, 0));
		grad.setColorAt(0, QColor(0, 0, 0, 0));
		grad.setColorAt(1, QColor(0, 0, 0, 255));

		QPainter painter(&alphaChannel_);
		painter.setBrush(grad);
		painter.setPen(Qt::NoPen);
		painter.drawRect(0, 0, 16, height());

		grad = QLinearGradient(QPointF(alphaChannel_.width() - 16, 0), QPointF(alphaChannel_.width(), 0));
		grad.setColorAt(0, QColor(0, 0, 0, 255));
		grad.setColorAt(1, QColor(0, 0, 0, 0));
		painter.setBrush(grad);
		painter.drawRect(alphaChannel_.width() - 16, 0, alphaChannel_.width(), height());
	}
	else {
		alphaChannel_.fill(QColor(0, 0, 0));
	}

	auto newScrollEnabled = (singleTextWidth_ > width() - leftMargin_);
	if (newScrollEnabled != scrollEnabled_) {
		updateText();
	}
}

void ScrollLabel::hideEvent(QHideEvent*) {
	if (scrollEnabled_) {
		scrollPos_ = 0;
		timer_.stop();
		wait_timer_.stop();
	}
}

void ScrollLabel::showEvent(QShowEvent*) {
	if (scrollEnabled_) {
		wait_timer_.stop();
		waiting_ = true;
	}
}