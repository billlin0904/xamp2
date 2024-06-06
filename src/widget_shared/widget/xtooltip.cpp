#include <widget/xtooltip.h>

XTooltip::XTooltip(const QString& text, QWidget* parent)
    : XDialog(parent, false) {
    setObjectName(qTEXT("XTooltip"));

    auto* client_widget = new QWidget(this);
    text_ = new QLabel(text, client_widget);

    switch (qTheme.themeColor()) {
	case ThemeColor::DARK_THEME:
        text_->setStyleSheet(qTEXT("QLabel { color: #FFFFFF; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #333333; border: 1px solid gray; border-radius: 8px; }"));
		break;
	case ThemeColor::LIGHT_THEME:
        text_->setStyleSheet(qTEXT("QLabel { color: #333333; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #FFFFFF; border: 1px solid gray; border-radius: 8px; }"));
        break;
    }    

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

bool XTooltip::eventFilter(QObject* obj, QEvent* e) {
    if (obj == this) {
        if (QEvent::WindowDeactivate == e->type()) {
            hide();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}