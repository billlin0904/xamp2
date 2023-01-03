#include <widget/xmessagebox.h>

#include <widget/widget_shared.h>
#include <widget/ui_utilts.h>

#include <QCheckBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QScopedPointer>

MaskWidget::MaskWidget(QWidget* parent)
	: QWidget(parent) {
	setWindowFlag(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_StyledBackground);
	// 255 * 0.7 = 178.5
	setStyleSheet(qTEXT("background-color: rgba(0, 0, 0, 179);"));
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
	const auto parent_rect = static_cast<QWidget*>(parent())->geometry();
	setGeometry(0, 0, parent_rect.width(), parent_rect.height());
}

XMessageBox::XMessageBox(const QString& title,
	const QString& text,
	QWidget* parent,
	const QFlags<QDialogButtonBox::StandardButton> buttons,
	const QDialogButtonBox::StandardButton default_button)
	: XDialog(parent) {
	button_box_ = new QDialogButtonBox(this);
	button_box_->setStandardButtons(QDialogButtonBox::StandardButtons(buttons));
	setDefaultButton(default_button);

	icon_label_ = new QLabel(this);
	message_text_label_ = new QLabel(this);

	icon_label_->setFixedSize(35, 35);
	icon_label_->setScaledContents(true);
	icon_label_->setStyleSheet(qTEXT("background: transparent;"));

	message_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	message_text_label_->setObjectName(qTEXT("messageTextLabel"));
	message_text_label_->setOpenExternalLinks(true);
	//message_text_label_->setFixedHeight(80);
	//message_text_label_->setFixedWidth(80);
	message_text_label_->setText(text);
	message_text_label_->setStyleSheet(qTEXT("background: transparent;"));

	auto* line = new QFrame(this);
	line->setFixedHeight(1);
	line->setFrameShape(QFrame::HLine);

	auto* client_widget = new QWidget(this);
	grid_layout_ = new QGridLayout(client_widget);
	grid_layout_->addWidget(icon_label_, 0, 0, 2, 1, Qt::AlignTop);
	grid_layout_->addWidget(message_text_label_, 0, 1, 2, 1);
	grid_layout_->addWidget(line, grid_layout_->rowCount(), 0, 1, grid_layout_->columnCount());
	grid_layout_->addWidget(button_box_, grid_layout_->rowCount(), 0, 1, grid_layout_->columnCount());
	grid_layout_->setSizeConstraint(QLayout::SetNoConstraint);
	grid_layout_->setHorizontalSpacing(0);
	grid_layout_->setVerticalSpacing(10);
	grid_layout_->setContentsMargins(10, 10, 10, 10);	

	QObject::connect(button_box_, &QDialogButtonBox::clicked, [this](auto* button) {
		onButtonClicked(button);
		});

	setContentWidget(client_widget);
	setTitle(title);

	connect(&timer_, &QTimer::timeout, this, &XMessageBox::updateTimeout);
	timer_.setInterval(1000);
	defaultButtonText_ = defaultButton()->text();
}

void XMessageBox::setText(const QString& text) {
	message_text_label_->setText(text);
}

void XMessageBox::onButtonClicked(QAbstractButton* button) {
	clicked_button_ = button;
	done(execReturnCode(button));
}

int XMessageBox::execReturnCode(QAbstractButton* button) {
	int nResult = button_box_->standardButton(button);
	return nResult;
}

void XMessageBox::setDefaultButton(QPushButton* button) {
	if (!button_box_->buttons().contains(button))
		return;

	default_button_ = button;
	button->setDefault(true);
	button->setFocus();
}

void XMessageBox::setDefaultButton(QDialogButtonBox::StandardButton button) {
	auto* default_button = button_box_->button(button);
	default_button->setObjectName(qTEXT("XMessageBoxDefaultButton"));

	QColor text_color;

	switch (qTheme.themeColor()) {
	case ThemeColor::LIGHT_THEME:
		text_color = Qt::white;
		break;
	default:
		text_color = qTheme.themeTextColor();
		break;
	}

	default_button->setStyleSheet(qSTR(
	R"(
      QPushButton#XMessageBoxDefaultButton {
		 background-color: %1;
		 color: %2;
      }
	)"
	).arg(colorToString(qTheme.highlightColor())).arg(colorToString(text_color)));
	setDefaultButton(default_button);
}

void XMessageBox::setIcon(const QIcon& icon) {
	icon_label_->setPixmap(icon.pixmap(35, 35));
}

QAbstractButton* XMessageBox::clickedButton() const {
	return clicked_button_;
}

QAbstractButton* XMessageBox::defaultButton() {
	return default_button_;
}

QDialogButtonBox::StandardButton XMessageBox::standardButton(QAbstractButton* button) const {
	return button_box_->standardButton(button);
}

void XMessageBox::addWidget(QWidget* widget) {
	grid_layout_->addWidget(widget, 1, 1, 2, 1);
}

QPushButton* XMessageBox::addButton(QDialogButtonBox::StandardButton buttons) {
	return button_box_->addButton(buttons);
}

void XMessageBox::showEvent(QShowEvent* event) {
	XDialog::showEvent(event);
	updateTimeout();
	timer_.start();
}

void XMessageBox::updateTimeout() {
	if (--timeout_ >= 0) {
		defaultButton()->setText(defaultButtonText_ + qSTR(" (%1)").arg(timeout_));
	} else {
		timer_.stop();
		defaultButton()->animateClick();
	}
}

void XMessageBox::showBug(const Exception& exception,
	const QString& title,
	QWidget* parent) {
	showButton(QString::fromStdString(exception.GetStackTrace()),
		title,
		qTheme.iconFromFont(0xF188),
		QDialogButtonBox::Ok,
		QDialogButtonBox::Ok,
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::showYesOrNo(const QString& text,
	const QString& title,
	QWidget* parent) {
	return showWarning(text,
		title,
		QDialogButtonBox::No | QDialogButtonBox::Yes,
		QDialogButtonBox::No,
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::showButton(const QString& text,
	const QString& title,
	const QIcon& icon,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	QScopedPointer<MaskWidget> mask_widget;
	if (!parent) {
		parent = qApp->activeWindow();
	}
	if (parent != nullptr) {
		parent->setFocus();
	}
	mask_widget.reset(new MaskWidget(parent));
	XMessageBox box(title, text, parent, buttons, default_button);
	box.setIcon(icon);
	if (box.exec() == -1)
		return QDialogButtonBox::Cancel;
	return box.standardButton(box.clickedButton());
}

QDialogButtonBox::StandardButton XMessageBox::showInformation(const QString& text,
	const QString& title,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showButton(text, title, qTheme.iconFromFont(Glyphs::ICON_MESSAGE_BOX_INFORMATION), buttons, default_button, parent);
}

QDialogButtonBox::StandardButton XMessageBox::showError(const QString& text,
	const QString& title,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showButton(text, title, qTheme.iconFromFont(Glyphs::ICON_MESSAGE_BOX_ERROR), buttons, default_button, parent);
}

QDialogButtonBox::StandardButton XMessageBox::showWarning(const QString& text,
	const QString& title,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showButton(text, title, qTheme.iconFromFont(Glyphs::ICON_MESSAGE_BOX_WARNING), buttons, default_button, parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::showCheckBoxQuestion(const QString& text,
	const QString& check_box_text,
	const QString& title,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showCheckBox(text, check_box_text, title, qTheme.iconFromFont(Glyphs::ICON_MESSAGE_BOX_QUESTION), buttons, default_button, parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::showCheckBoxInformation(const QString& text,
	const QString& check_box_text,
	const QString& title,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showCheckBox(text, check_box_text, title, qTheme.iconFromFont(Glyphs::ICON_MESSAGE_BOX_INFORMATION), buttons, default_button, parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::showCheckBox(const QString& text,
	const QString& check_box_text,
	const QString& title,
	const QIcon& icon,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	if (!parent) {
		parent = qApp->activeWindow();
	}
	if (parent != nullptr) {
		parent->setFocus();
	}
	QScopedPointer<MaskWidget> mask_widget(new MaskWidget(parent));
	XMessageBox box(title, text, parent, buttons);
	box.setIcon(icon);
	auto* check_box = new QCheckBox(&box);
	check_box->setStyleSheet(qTEXT("background: transparent;"));
	check_box->setText(check_box_text);
	box.addWidget(check_box);
	if (box.exec() == -1) {
		return { QDialogButtonBox::Cancel, false };
	}
	const auto standard_button = box.standardButton(box.clickedButton());
	if (standard_button == QMessageBox::Ok) {
		return { QDialogButtonBox::Yes, check_box->isChecked() };
	}
	return { QDialogButtonBox::Cancel, check_box->isChecked() };
}