#include <widget/xtooltip.h>

XTooltip::XTooltip(const QString& text, QWidget* parent)
    : XDialog(parent, false) {
    setObjectName(qTEXT("XTooltip"));

	(void)QObject::connect(&timer_, &QTimer::timeout, this, &XTooltip::hide);

    auto* client_widget = new QWidget(this);
    text_ = new QLabel(text, client_widget);

	onThemeChangedFinished(qTheme.themeColor());    

    auto* layout = new QVBoxLayout(client_widget);
    layout->addWidget(text_);
    client_widget->setLayout(layout);    
    setFixedHeight(45);

    setContentWidget(client_widget, false, true, true);
    installEventFilter(this);
}

void XTooltip::setText(const QString& text) {
    text_->setText(text);
}

QString XTooltip::text() const {
    return text_->text();
}

void XTooltip::showAndStart() {
	show();
	timer_.start(2000);
}

void XTooltip::onThemeChangedFinished(ThemeColor theme_color) {
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        text_->setStyleSheet(qTEXT("QLabel { color: #FFFFFF; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #333333; border: 1px solid gray; border-radius: 8px; }"));
        break;
    case ThemeColor::LIGHT_THEME:
        text_->setStyleSheet(qTEXT("QLabel { color: #333333; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #FFFFFF; border: 1px solid gray; border-radius: 8px; }"));
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