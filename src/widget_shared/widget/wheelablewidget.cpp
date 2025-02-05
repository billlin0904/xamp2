#include <widget/wheelablewidget.h>

#include <QScroller>
#include <QPainter>
#include <QKeyEvent>

#include <widget/util/str_util.h>

WheelableWidget::WheelableWidget(const bool touch, QWidget* parent)
    : QWidget(parent)
    , is_scrolled_(false)
	, do_signal_(true)
	, item_(0)
	, item_offset_(0)
	, mask_length_(0) {
	QScroller::grabGesture(this, touch ? QScroller::TouchGesture : QScroller::LeftMouseButtonGesture);
}

int32_t WheelableWidget::currentIndex() const {
	return item_;
}

void WheelableWidget::setCurrentIndex(const int32_t index) {
	if (index >= 0 && index < itemCount()) {
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

	// �C�� item ���� (���] itemHeight() �^�Ǫ��O���t�B�~��Z)
	const auto iH = itemHeight() + 10;
	const auto iC = itemCount();

	// �� 1) �� qMax(1, ...) ��ø�s�d��ܤ֥]�t 1
	const auto halfRange = qMax(1, h / 2 / iH);

	if (iC > 0) {
		// �� 2) �����νվ�쥻 if (!len) return; ����������
		// �Ҧp�����g��:
		// if (halfRange <= 0) {
		//     // �z�פW�o���|�i�ӤF�A�]�� halfRange �ܤַ|�O 1
		// }

		// �� 3) �ϥ� halfRange �M���nø�s�� index offset
		for (auto i = -halfRange; i <= halfRange; i++) {
			const auto item_num = item_ + i;

			// ����ˬd�G�u���b�d�򤺤~ø
			if (item_num >= 0 && item_num < iC) {

				// �o�̫O�d�쥻���H�p�⪺�{���X
				// ---------------------------------------------------
				// �ھڥثe�� i �Ӱ��z���שΤj�p�ܤ�
				const auto len = halfRange;
				auto t = std::abs(i);
				int alpha = 255 - t * t * 220 / (len * len);
				if (alpha < 0) alpha = 0;

				painter.setPen(QColor(255, 255, 255, alpha));

				// �p��ø�s��m
				QRect rect(0, h / 2 + i * iH - item_offset_, w, iH);

				// �I�s�A���ۭq�禡ø�s�� item
				paintItem(&painter, item_num, rect);
			}
		}
	}

	// ø�s��L�B�n�B�ؽu��
	paintItemMask(&painter);
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

void WheelableWidget::onScrollTo(const int32_t index) {
	do_signal_ = false;
	auto* scroller = QScroller::scroller(this);
	scroller->scrollTo(QPointF(0, kWheelScrollOffset + index * itemHeight()), kScrollTime);
}

bool WheelableWidget::event(QEvent* event) {
	switch (event->type()) {
	case QEvent::ScrollPrepare: {
			auto* scroll_prepare_event = dynamic_cast<QScrollPrepareEvent *>(event);
			scroll_prepare_event->setViewportSize(QSizeF(size()));
			scroll_prepare_event->setContentPosRange(QRectF(0.0, 0.0, 0.0, kWheelScrollOffset * 2));
			scroll_prepare_event->setContentPos(QPointF(0.0, kWheelScrollOffset + item_ * itemHeight() + item_offset_));
			event->accept();
			return true;
		}
		break;
	case QEvent::Scroll: {
			const auto scroll_event = dynamic_cast<QScrollEvent *>(event);
			const auto y = scroll_event->contentPos().y();
			const int32_t iy = y - kWheelScrollOffset;
			const auto ih = itemHeight();
			const auto ic = itemCount();
			if (ic > 0) {
				item_ = iy / ih;
				item_offset_ = iy % ih;

				if (item_ >= ic)
					item_ = ic - 1;
			}

			if (do_signal_) {
				if (scroll_event->scrollState() == QScrollEvent::ScrollStarted) {
					if (item_ >= 0)
						is_scrolled_ = true;
				}
			}

			if (scroll_event->scrollState() == QScrollEvent::ScrollFinished) {
				if (do_signal_) {
					if (item_>1)
						emit changeTo(item_ + 1);
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
		break;
	}
	return QWidget::event(event);
}
