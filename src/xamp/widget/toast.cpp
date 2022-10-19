#include <QPropertyAnimation>
#include <QTimer>
#include <QGuiApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QPainter>

#include <widget/ui_utilts.h>
#include <widget/toast.h>

Toast::Toast(QWidget* parent)
	: QWidget(parent) {
	ui.setupUi(this);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_StyledBackground, true);
    qTheme.setBackgroundColor(this);
}

Toast::~Toast() {
}

void Toast::setText(const QString& text) {
	ui.label->setText(text);
}

void Toast::showAnimation(int timeout) {
	auto animation = new QPropertyAnimation(this, "windowOpacity");
	animation->setDuration(1000);
	animation->setStartValue(0);
	animation->setEndValue(1);
	animation->start();
	show();
	QTimer::singleShot(timeout, [&]() {
		hideAnimation();
		});
}

void Toast::hideAnimation() {
	auto animation = new QPropertyAnimation(this, "windowOpacity");
	animation->setDuration(1000);
	animation->setStartValue(1);
	animation->setEndValue(0);
	animation->start();
	(void)QObject::connect(animation, &QPropertyAnimation::finished, [&]() {
		close();
		deleteLater();
		});
}

void Toast::showTip(const QString& text, QWidget* parent) {
	auto toast = new Toast(parent);
	toast->setWindowFlags(toast->windowFlags() | Qt::WindowStaysOnTopHint);
	toast->setText(text);
	toast->adjustSize();
	centerDesktop(toast);
	toast->showAnimation();
}

void Toast::paintEvent(QPaintEvent* /*event*/) {
	auto kBackgroundColor = QColor(228, 233, 237);

	QPainter paint(this);	
	kBackgroundColor.setAlpha(0.0 * 255);

	paint.setRenderHint(QPainter::Antialiasing, true);
	paint.setPen(Qt::NoPen);
	paint.setBrush(QBrush(kBackgroundColor, Qt::SolidPattern));

	paint.drawRect(0, 0, width(), height());
}
