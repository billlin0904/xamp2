#include <QVariant>
#include <QEvent>
#include <widget/tooltipsfilter.h>

ToolTipsFilter::ToolTipsFilter(QObject* parent)
	: QObject(parent)
	, timer_(this)
	, parent_(nullptr)
	, tooltip_(nullptr) {
	(void) QObject::connect(&timer_, &QTimer::timeout, this, [this]() {
		if (parent_ != nullptr) {
			auto *hint = parent_->property("ToolTip").value<QWidget*>();
			showTooltip(hint);
		}
		});
	timer_.setInterval(500);
}

bool ToolTipsFilter::eventFilter(QObject* obj, QEvent* event) {
	switch (event->type()) {
	case QEvent::Enter: {
		if (tooltip_) {
			tooltip_->hide();
		}

		auto parent = qobject_cast<QWidget*>(obj);
		parent_ = parent;
		if (!parent_) {
			break;
		}

		tooltip_ = parent_->property("ToolTip").value<QWidget*>();
		if (!tooltip_) {
			break;
		}
		timer_.stop();
		timer_.start();
		parent_->setCursor(QCursor(Qt::ArrowCursor));
		}
		break;
	case QEvent::Leave: {
		if (tooltip_ != nullptr) {
			tooltip_->hide();
			timer_.stop();
		}
		auto parent = qobject_cast<QWidget*>(obj);
		if (parent != nullptr) {
			parent->unsetCursor();
		}
		}
		break;
	case QEvent::MouseButtonPress:
		if (tooltip_ != nullptr) {
			tooltip_->hide();
			timer_.stop();
		}
		break;
	}
	return QObject::eventFilter(obj, event);
}

void ToolTipsFilter::showTooltip(QWidget* tooltip) {
	if (!parent_) {
		return;
	}
	tooltip_ = tooltip;

	if (tooltip_ != nullptr && tooltip_ != tooltip) {
		tooltip_->hide();
	}

	QTimer::singleShot(10, [=]() {
		auto center_pos = parent_->mapToGlobal(parent_->rect().center());
		
		tooltip_->setWindowFlags(tooltip_->windowFlags() | Qt::WindowStaysOnTopHint);
		tooltip_->show();
		tooltip_->adjustSize();

		auto sz = tooltip_->size();
		center_pos.setX(center_pos.x() - sz.width() / 2);
		center_pos.setY(center_pos.y() - 32 - sz.height());
		center_pos = tooltip_->mapFromGlobal(center_pos);
		center_pos = tooltip_->mapToParent(center_pos);
		tooltip_->move(center_pos);
		tooltip_->raise();
		});
}