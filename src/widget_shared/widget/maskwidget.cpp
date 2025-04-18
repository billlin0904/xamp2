#include <QPropertyAnimation>
#include <widget/maskwidget.h>
#include <widget/util/str_util.h>

MaskWidget::MaskWidget(QWidget* parent)
	: QWidget(parent) {
	setWindowFlag(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_StyledBackground);
	// 255 * 0.7 = 178.5
	setStyleSheet("background-color: rgba(0, 0, 0, 179);"_str);
	auto* animation = new QPropertyAnimation(this, "windowOpacity");
	animation->setDuration(2000);
	animation->setEasingCurve(QEasingCurve::OutBack);
	animation->setStartValue(0.0);
	animation->setEndValue(1.0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
	show();
}

void MaskWidget::showEvent(QShowEvent* event) {
	if (!parent()) {
		return;
	}
	const auto parent_rect = dynamic_cast<QWidget*>(parent())->geometry();
	setGeometry(0, 0, parent_rect.width(), parent_rect.height());
}
