#include <QPainter>
#include <widget/scrolllabel.h>

ScrollLabel::ScrollLabel(QWidget* parent) 
	: QLabel(parent) {
    static_text_.setTextFormat(Qt::PlainText);
	timer_.setInterval(16);
	timer_.setTimerType(Qt::PreciseTimer);
	(void)QObject::connect(&timer_, &QTimer::timeout, this, &ScrollLabel::onTimerTimeout);
    wait_timer_.setInterval(100);
	(void)QObject::connect(&wait_timer_, &QTimer::timeout, this, &ScrollLabel::onTimerTimeout);
	//left_margin_ = height() / 3;
	left_margin_ = 0;
	scroll_pos_ = 0;
	scroll_enabled_ = false;
	waiting_ = true;	
	updateText();
}

void ScrollLabel::setText(const QString& text) {
	text_ = text;
	scroll_pos_ = 0;
	updateText();
	update();
	updateGeometry();
}

QString ScrollLabel::text() const {
	return text_;
}

void ScrollLabel::updateText() {
	timer_.stop();

	single_text_width_ = fontMetrics().width(text_);
    scroll_enabled_ = single_text_width_ > (width() - left_margin_);

	if (scroll_enabled_) {
        static_text_.setText(text_ + seperator);
        if (!(window()->windowState() & Qt::WindowMinimized)) {
			scroll_pos_ = 0;
			wait_timer_.start();
			waiting_ = true;
		}
	}
	else {
        static_text_.setText(text_);
	}

    static_text_.prepare(QTransform(), font());
    whole_text_size_ = QSize(fontMetrics().width(static_text_.text()), fontMetrics().height());
}

void ScrollLabel::onTimerTimeout() {
	if (whole_text_size_.width() == 0) {
		return;
	}

	scroll_pos_ = (scroll_pos_ + 1) % whole_text_size_.width();

	if (waiting_) {
		waiting_ = false;
		timer_.start();
		wait_timer_.stop();
	}

	if (scroll_pos_ == 0) {
		waiting_ = true;
		timer_.stop();
		wait_timer_.start();
	}

    update();
}

QSize ScrollLabel::sizeHint() const {
	return QSize(std::min(whole_text_size_.width() + left_margin_, maximumWidth()), fontMetrics().height());
}

void ScrollLabel::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	if (scroll_enabled_) {
		buffer_.fill(qRgba(0, 0, 0, 0));
		QPainter pb(&buffer_);
		pb.setPen(painter.pen());

		pb.setRenderHint(QPainter::Antialiasing);
		auto ui_font = font();
		ui_font.setStyleStrategy(QFont::PreferQuality);
		ui_font.setHintingPreference(QFont::PreferFullHinting);
		pb.setFont(ui_font);

		auto x = std::min(-scroll_pos_, 0) + left_margin_;
		while (x < width()) {
            pb.drawStaticText(QPointF(x, (height() - whole_text_size_.height()) / 2), static_text_);
			x += whole_text_size_.width();
		}
		pb.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		pb.setClipRect(width() - 15, 0, 15, height());
		pb.drawImage(0, 0, alpha_channel_);
		pb.setClipRect(0, 0, 15, height());
		pb.drawImage(0, 0, alpha_channel_);
		painter.drawImage(0, 0, buffer_);
	}
	else {
        painter.drawStaticText(QPointF(left_margin_, (height() - whole_text_size_.height()) / 2), static_text_);
	}
}

void ScrollLabel::resizeEvent(QResizeEvent*) {
	alpha_channel_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
	buffer_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
	alpha_channel_.fill(qRgba(0, 0, 0, 0));
	buffer_.fill(qRgba(0, 0, 0, 0));

	if (width() > 64) {
		QLinearGradient grad(QPointF(0, 0), QPointF(16, 0));
		grad.setColorAt(0, QColor(0, 0, 0, 0));
		grad.setColorAt(1, QColor(0, 0, 0, 255));

		QPainter painter(&alpha_channel_);
		painter.setBrush(grad);
		painter.setPen(Qt::NoPen);
		painter.drawRect(0, 0, 16, height());

		grad = QLinearGradient(QPointF(alpha_channel_.width() - 16, 0), QPointF(alpha_channel_.width(), 0));
		grad.setColorAt(0, QColor(0, 0, 0, 255));
		grad.setColorAt(1, QColor(0, 0, 0, 0));
		painter.setBrush(grad);
		painter.drawRect(alpha_channel_.width() - 16, 0, alpha_channel_.width(), height());
	}
	else {
		alpha_channel_.fill(QColor(0, 0, 0));
	}

	const auto new_scroll_enabled = (single_text_width_ > width() - left_margin_);
	if (new_scroll_enabled != scroll_enabled_) {
		updateText();
	}
}

void ScrollLabel::hideEvent(QHideEvent*) {	
}

void ScrollLabel::showEvent(QShowEvent*) {
}
