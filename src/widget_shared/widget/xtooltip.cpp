#include <QStyleOption>
#include <QPainter>

#include <widget/xtooltip.h>

XTooltip::XTooltip(const QString& text, QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("XTooltip"_str);
	(void)QObject::connect(&timer_, &QTimer::timeout, this, &XTooltip::hide);
    text_ = new QLabel(this);
	text_->setObjectName("textLabel"_str);
	image_ = new QLabel(this);
    image_->hide();
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(image_, 0, Qt::AlignCenter);
    layout->addWidget(text_, 1, Qt::AlignCenter);
    layout->setSpacing(0);
    layout->setContentsMargins(5, 5, 5, 5);
    text_->setFixedHeight(18);
    setLayout(layout);
    setFixedHeight(45);
    installEventFilter(this);
	setText(text);
    onThemeChangedFinished(qTheme.themeColor());
}

void XTooltip::setImageSize(const QSize& size) {
    image_->setMinimumSize(size);
    setFixedHeight(size.height() + 45);
}

void XTooltip::setImage(const QPixmap& pixmap) {
	image_->setPixmap(pixmap);
	image_->show();
}

void XTooltip::setTextAlignment(Qt::Alignment alignment) {
	text_->setAlignment(alignment);
	adjustSize();
}

void XTooltip::setTextFont(const QFont& font) {
	text_->setFont(font);
	adjustSize();
}

void XTooltip::setText(const QString& text) {
    text_->setText(text);
    QFontMetrics metrics(text_->font());
    max_width_ = (std::max)(max_width_, metrics.horizontalAdvance(text));
    text_->setMinimumWidth(max_width_ + 10);
    image_->setMinimumWidth(max_width_ + 10);
	adjustSize();
}

QString XTooltip::text() const {
    return text_->text();
}

void XTooltip::showAndStart(bool start_timer) {
	show();
    timer_.stop();
    if (start_timer) {
        timer_.start(500);
    }
    update();
}

void XTooltip::onThemeChangedFinished(ThemeColor theme_color) {
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        text_->setStyleSheet("QLabel#textLabel { color: #FFFFFF; font-size: 14px; background: transparent; }"_str);
        setStyleSheet("XTooltip { background-color: #424548; border: 1px solid #4d4d4d; border-radius: 8px; }"_str);
        break;
    case ThemeColor::LIGHT_THEME:
        text_->setStyleSheet("QLabel#textLabel { color: #2e2f31; font-size: 14px; background: transparent; }"_str);
        setStyleSheet("XTooltip { background-color: #e6e6e6; border: 1px solid transparent; border-radius: 8px; }"_str);
        break;
    }
}

bool XTooltip::eventFilter(QObject* obj, QEvent* e) {
    if (obj == this) {
        if (QEvent::WindowDeactivate == e->type()) {
            hide();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

void XTooltip::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
