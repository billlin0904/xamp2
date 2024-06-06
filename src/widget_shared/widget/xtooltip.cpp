#include <widget/xtooltip.h>

XTooltip::XTooltip(const QString& text, QWidget* parent)
    : XDialog(parent, true) {
    setObjectName(qTEXT("XTooltip"));

    (void)QObject::connect(&timer_, &QTimer::timeout, this, &XTooltip::close);

    auto* client_widget = new QWidget(this);
    auto* label = new QLabel(text, client_widget);

    switch (qTheme.themeColor()) {
	case ThemeColor::DARK_THEME:
		label->setStyleSheet(qTEXT("QLabel { color: #FFFFFF; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #333333; border: 1px solid gray; border-radius: 8px; }"));
		break;
	case ThemeColor::LIGHT_THEME:
		label->setStyleSheet(qTEXT("QLabel { color: #333333; font-size: 14px; background: transparent; }"));
        setStyleSheet(qTEXT("XTooltip { background-color: #FFFFFF; border: 1px solid gray; border-radius: 8px; }"));
        break;
    }    

    auto* layout = new QVBoxLayout(client_widget);
    layout->addWidget(label);
    client_widget->setLayout(layout);    
    setFixedHeight(45);

    setContentWidget(client_widget, false, true, true);
    installEventFilter(this);       

	timer_.start(1000);
}

bool XTooltip::eventFilter(QObject* obj, QEvent* e) {
    if (obj == this) {
        if (QEvent::WindowDeactivate == e->type()) {
            this->close();
            e->accept();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}