#include <QStyleOption>
#include <QPainter>

#include <widget/xtooltip.h>

XTooltip::XTooltip(const QString& text, QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName(qTEXT("XTooltip"));
	(void)QObject::connect(&timer_, &QTimer::timeout, this, &XTooltip::hide);
    text_ = new QLabel(this);
	text_->setObjectName(qTEXT("textLabel"));
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(text_);
    setLayout(layout);
    setFixedHeight(45);    
    installEventFilter(this);
	setText(text);
    onThemeChangedFinished(qTheme.themeColor());
}

void XTooltip::setText(const QString& text) {
    text_->setText(text);
    QFontMetrics metrics(text_->font());
    max_width_ = (std::max)(max_width_, metrics.horizontalAdvance(text));
    text_->setMinimumWidth(max_width_ + 10);
	adjustSize();
}

QString XTooltip::text() const {
    return text_->text();
}

void XTooltip::showAndStart() {    
	show();
	timer_.stop();
	timer_.start(1000);    
}

void XTooltip::onThemeChangedFinished(ThemeColor theme_color) {
    // FIXME: border-radius not work.
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        text_->setStyleSheet(qTEXT("QLabel#textLabel { color: #FFFFFF; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #424548; border: 1px solid #4d4d4d; border-radius: 8px; }"));
        break;
    case ThemeColor::LIGHT_THEME:
        text_->setStyleSheet(qTEXT("QLabel#textLabel { color: #2e2f31; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #e6e6e6; border: 1px solid transparent; border-radius: 8px; }"));
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
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
