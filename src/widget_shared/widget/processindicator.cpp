#include <widget/processindicator.h>

#include <QPainter>

#include <thememanager.h>
#include <widget/ui_utilts.h>

ProcessIndicator::ProcessIndicator(QWidget* parent)
	: QWidget(parent)
	, angle_(0)
	, timer_id_(-1)
	, delay_(40)
	, displayed_when_stopped_(false)
	, color_(Qt::black) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    centerParent(this);

    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        setColor(Qt::white);
        break;
    case ThemeColor::LIGHT_THEME:
        setColor(Qt::black);
        break;
    }
    show();
}

bool ProcessIndicator::isAnimated() const {
	return (timer_id_ != -1);
}

void ProcessIndicator::setDisplayedWhenStopped(bool state) {
	displayed_when_stopped_ = state;
	update();
}

bool ProcessIndicator::isDisplayedWhenStopped() const {
	return displayed_when_stopped_;
}

void ProcessIndicator::startAnimation() {
	angle_ = 0;

	if (timer_id_ == -1)
		timer_id_ = startTimer(delay_);
}

void ProcessIndicator::stopAnimation() {
    if (timer_id_ != -1)
        killTimer(timer_id_);

    timer_id_ = -1;
    update();
}

void ProcessIndicator::setAnimationDelay(int delay) {
    if (timer_id_ != -1)
        killTimer(timer_id_);

    delay_ = delay;
    if (timer_id_ != -1)
        timer_id_ = startTimer(delay_);
}

void ProcessIndicator::setColor(const QColor& color) {
    color_ = color;
    update();
}

QSize ProcessIndicator::sizeHint() const {
    return QSize(25, 25);
}

int ProcessIndicator::heightForWidth(int w) const {
    return w;
}

void ProcessIndicator::setStoppedIcon(const QIcon& icon) {
    stopped_icon_ = icon;
}

void ProcessIndicator::timerEvent(QTimerEvent* /*event*/) {
    angle_ = (angle_ + 30) % 360;
    update();
    centerParent(this);
}

void ProcessIndicator::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);

    const auto width = qMin(this->width(), this->height());

    if (!displayed_when_stopped_ && !isAnimated()) {
        if (!stopped_icon_.isNull()) {
            auto pixmap = stopped_icon_.pixmap(QSize(24, 24));
            p.drawPixmap(0, 0, pixmap);
        }        
        return;
    }

    p.setRenderHints(QPainter::Antialiasing, true);
    p.setRenderHints(QPainter::SmoothPixmapTransform, true);
    p.setRenderHints(QPainter::TextAntialiasing, true);

    const auto outer_radius = (width - 1) * 0.5;
    const auto inner_radius = (width - 1) * 0.5 * 0.38;

    const auto capsule_height = outer_radius - inner_radius;
    const auto capsule_width = (width > 32) ? capsule_height * .23 : capsule_height * .35;
    const auto capsule_radius = capsule_width / 2;

    for (auto i = 0; i < 12; i++) {
        QColor color = color_;
        color.setAlphaF(1.0f - (i / 12.0f));
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.save();
        p.translate(rect().center());
        p.rotate(angle_ - i * 30.0f);
        p.drawRoundedRect(-capsule_width * 0.5, -(inner_radius + capsule_height), capsule_width, capsule_height, capsule_radius, capsule_radius);
        p.restore();
    }
}