#include <widget/seekslider.h>

#include <QStyle>
#include <thememanager.h>
#include <widget/util/str_util.h>

SeekSlider::SeekSlider(QWidget* parent)
	: QSlider(parent) {
	animation_ = new QPropertyAnimation(this, "value");
}

void SeekSlider::setRange(int64_t min, int64_t max) {
	min_ = min;
	max_ = max;
	QSlider::setRange(static_cast<int>(min), static_cast<int>(max));
}

void SeekSlider::enableAnimation(bool enable) {
	if (!enable && animation_) {
		animation_->deleteLater();
		animation_ = nullptr;
	}
}

void SeekSlider::setValueAnimation(int value, bool animate) {
	target_ = value;
	if (animate && animation_ != nullptr) {
		animation_->stop();
		animation_->setDuration(duration_);
		animation_->setEasingCurve(easing_curve_);
		animation_->setStartValue(QSlider::value());
		animation_->setEndValue(value);
		animation_->start();
		return;
	}
	QSlider::setValue(value);
}

void SeekSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        event->accept();

        int64_t value = 0;
        if (orientation() == Qt::Horizontal) {
            const int x = event->pos().x();
            if (width() > 0) {
                value = ((max_ - min_) * x / width()) + min_;
            }
        }
        else {
            const int y = event->pos().y();
            if (height() > 0) {
                value = ((max_ - min_) * (height() - y) / height()) + min_;
            }
        }

        // ���b [min_, max_] �d��
        value = qBound<int64_t>(min_, value, max_);

        setValueAnimation(static_cast<int>(value), true);
        emit leftButtonValueChanged(value);

        // �� ���ݨD�M�w�O�_�I�s�����O, �Ϊ̦b�}�l�ɩI�s
        // QSlider::mousePressEvent(event);
        return;
    }
    QSlider::mousePressEvent(event);
}

//void SeekSlider::enterEvent(QEnterEvent* event) {
//	qTheme.setSliderTheme(this, true);
//}
//
//void SeekSlider::leaveEvent(QEvent* event) {
//	qTheme.setSliderTheme(this, false);
//}
//void SeekSlider::wheelEvent(QWheelEvent* event) {
//	constexpr int kVolumeSensitivity = 30;
//	const uint step = event->angleDelta().y() / kVolumeSensitivity;
//	setValueAnimation(value() + step, true);
//}