#include <QScroller>
#include <QPainter>
#include <QKeyEvent>

#include <widget/str_utilts.h>
#include <widget/wheelablewidget.h>

WheelableWidget::WheelableWidget(const bool touch, QWidget* parent)
    : QWidget(parent)
    , is_scrolled_(false)
	, do_signal_(true)
	, item_(0)
	, item_offset_(0)
	, mask_length_(0) {
	QScroller::grabGesture(this, touch ? QScroller::TouchGesture : QScroller::LeftMouseButtonGesture);
}

int32_t WheelableWidget::CurrentIndex() const {
	return item_;
}

void WheelableWidget::SetCurrentIndex(const int32_t index) {
	if (index >= 0 && index < ItemCount()) {
		item_ = index;
		item_offset_ = 0;
		update();
	}
}

void WheelableWidget::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);

	const auto w = width();
	const auto h = height();

	const auto iH = ItemHeight();
	const auto iC = ItemCount();

	if (iC > 0) {
		for (auto i = -h / 2 / iH; i <= h / 2 / iH; i++) {
			const auto item_num = item_ + i;

			if (item_num >= 0 && item_num < iC) {
				const auto len = h / 2 / iH;
				auto t = abs(i);

				if (!len) {
					return;
				}

				t = 255 - t * t * 220 / len / len;

				if (t < 0)
					t = 0;

				painter.setPen(QColor(255, 255, 255, t));
				QRect rect(0, h / 2 + i * iH - item_offset_, w, iH);
				PaintItem(&painter, item_num, rect);
			}
		}
	}

	PaintItemMask(&painter);
	//paintBackground(&painter);
}

void WheelableWidget::mousePressEvent(QMouseEvent* event) {
	QWidget::mousePressEvent(event);
}

void WheelableWidget::mouseMoveEvent(QMouseEvent* event) {
	QWidget::mouseMoveEvent(event);
}

void WheelableWidget::mouseReleaseEvent(QMouseEvent* event) {
	QWidget::mouseReleaseEvent(event);
}

void WheelableWidget::ScrollTo(const int32_t index) {
	do_signal_ = false;
	auto scroller = QScroller::scroller(this);
	scroller->scrollTo(QPointF(0, kWheelScrollOffset + index * ItemHeight()), kScrollTime);
}

bool WheelableWidget::event(QEvent* event) {
	switch (event->type()) {
	case QEvent::ScrollPrepare: {
			auto scroll_prepare_event = dynamic_cast<QScrollPrepareEvent *>(event);
			scroll_prepare_event->setViewportSize(QSizeF(size()));
			scroll_prepare_event->setContentPosRange(QRectF(0.0, 0.0, 0.0, kWheelScrollOffset * 2));
			scroll_prepare_event->setContentPos(QPointF(0.0, kWheelScrollOffset + item_ * ItemHeight() + item_offset_));
			event->accept();
			return true;
		}
		break;
	case QEvent::Scroll: {
			const auto scroll_event = dynamic_cast<QScrollEvent *>(event);
			const auto y = scroll_event->contentPos().y();
			const int32_t iy = y - kWheelScrollOffset;
			const auto ih = ItemHeight();
			const auto ic = ItemCount();
			if (ic > 0) {
				item_ = iy / ih;
				item_offset_ = iy % ih;

				if (item_ >= ic)
					item_ = ic - 1;
			}

			if (do_signal_) {
				if (scroll_event->scrollState() == QScrollEvent::ScrollStarted) {
					if (item_ > 1)
						is_scrolled_ = true;
				}
			}

			if (scroll_event->scrollState() == QScrollEvent::ScrollFinished) {
				if (do_signal_) {
					if (item_>1)
						emit ChangeTo(item_ + 1);
					mask_length_ = 0;
					real_current_text_.clear();
				}
				is_scrolled_ = false;
				do_signal_ = true;
				item_offset_ = 0;
			}

			if (item_offset_ != 0)
				update();

			scroll_event->accept();
			return true;
		}
		break;
	case QEvent::MouseButtonPress:
		setFocus();
		return true;
	default:
		return QWidget::event(event);
	}
	return true;
}
