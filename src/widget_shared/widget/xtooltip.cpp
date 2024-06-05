#include <widget/xtooltip.h>

XTooltip::XTooltip(const QString& text, QWidget* parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    //setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    setObjectName(qTEXT("XTooltip"));

    (void)QObject::connect(&timer_, &QTimer::timeout, this, &XTooltip::close);

    auto* label = new QLabel(text, this);

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

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->setContentsMargins(15, 10, 15, 10);

    setLayout(layout);	   

    auto* effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(20);
    effect->setOffset(0, 5);
    effect->setColor(QColor(0, 0, 0, 160));
    setGraphicsEffect(effect);
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