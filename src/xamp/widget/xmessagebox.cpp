#include <widget/xmessagebox.h>

#include <widget/widget_shared.h>
#include <widget/xmainwindow.h>
#include <widget/ui_utilts.h>

#include <QApplication>
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
	const QDialogButtonBox::StandardButton default_button,
	bool enable_countdown)
	: XDialog(parent) {
	enable_countdown_ = enable_countdown;

	button_box_ = new QDialogButtonBox(this);
	button_box_->setStandardButtons(QDialogButtonBox::StandardButtons(buttons));
	SetDefaultButton(default_button);

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
		OnButtonClicked(button);
		});

	SetContentWidget(client_widget);
	SetTitle(title);

	connect(&timer_, &QTimer::timeout, this, &XMessageBox::UpdateTimeout);
	timer_.setInterval(1000);
	default_button_text_ = DefaultButton()->text();
}

void XMessageBox::SetTextFont(const QFont& font) {
	message_text_label_->setFont(font);
}

void XMessageBox::SetText(const QString& text) {
	message_text_label_->setText(text);
}

void XMessageBox::OnButtonClicked(QAbstractButton* button) {
	clicked_button_ = button;
	done(ExecReturnCode(button));
}

int XMessageBox::ExecReturnCode(QAbstractButton* button) {
	int nResult = button_box_->standardButton(button);
	return nResult;
}

void XMessageBox::SetDefaultButton(QPushButton* button) {
	if (!button_box_->buttons().contains(button))
		return;

	default_button_ = button;
	button->setDefault(true);
	button->setFocus();
}

void XMessageBox::SetDefaultButton(QDialogButtonBox::StandardButton button) {
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
	SetDefaultButton(default_button);
}

void XMessageBox::SetIcon(const QIcon& icon) {
	icon_label_->setPixmap(icon.pixmap(35, 35));
}

QAbstractButton* XMessageBox::ClickedButton() const {
	return clicked_button_;
}

QAbstractButton* XMessageBox::DefaultButton() {
	return default_button_;
}

QDialogButtonBox::StandardButton XMessageBox::StandardButton(QAbstractButton* button) const {
	return button_box_->standardButton(button);
}

void XMessageBox::AddWidget(QWidget* widget) {
	grid_layout_->addWidget(widget, 1, 1, 2, 1);
}

QPushButton* XMessageBox::AddButton(QDialogButtonBox::StandardButton buttons) {
	return button_box_->addButton(buttons);
}

void XMessageBox::showEvent(QShowEvent* event) {
	XDialog::showEvent(event);
	if (!enable_countdown_) {
		return;
	}
	UpdateTimeout();
	timer_.start();
}

void XMessageBox::UpdateTimeout() {
	if (--timeout_ >= 0) {
		DefaultButton()->setText(default_button_text_ + qSTR(" (%1)").arg(timeout_));
	} else {
		timer_.stop();
		DefaultButton()->animateClick();
	}
}

void XMessageBox::ShowBug(const Exception& exception,
	const QString& title,
	QWidget* parent) {
	XMessageBox box(title, QString::fromStdString(exception.GetStackTrace()), 
		parent, 
		QDialogButtonBox::Ok,
		QDialogButtonBox::Ok,
		false);
	box.SetIcon(qTheme.fontIcon(0xF188));
	box.SetTextFont(QFont(qTEXT("DebugFont")));
	box.exec();
}

QDialogButtonBox::StandardButton XMessageBox::ShowYesOrNo(const QString& text,
	const QString& title,
	bool enable_countdown,
	QWidget* parent) {
	return ShowWarning(text,
		title,
		enable_countdown,
		QDialogButtonBox::No | QDialogButtonBox::Yes,
		QDialogButtonBox::No,
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::ShowButton(const QString& text,
	const QString& title,
	bool enable_countdown,
	const QIcon& icon,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	QScopedPointer<MaskWidget> mask_widget;
    if (!parent) {
		parent = GetMainWindow();
	}
	if (parent != nullptr) {
		parent->setFocus();
	}
	mask_widget.reset(new MaskWidget(parent));
	XMessageBox box(title, text, parent, buttons, default_button, enable_countdown);
	// Note: Don't call centerParent(), centerDesktop()
	box.SetIcon(icon);
	if (box.exec() == -1)
		return QDialogButtonBox::Cancel;
	return box.StandardButton(box.ClickedButton());
}

QDialogButtonBox::StandardButton XMessageBox::ShowInformation(const QString& text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return ShowButton(text, 
		title, 
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_INFORMATION),
		buttons,
		default_button,
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::ShowError(const QString& text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return ShowButton(text, 
		title,
		enable_countdown, 
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_ERROR),
		buttons, 
		default_button, 
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::ShowWarning(const QString& text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return ShowButton(text,
		title,
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_WARNING),
		buttons,
		default_button,
		parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::ShowCheckBoxQuestion(const QString& text,
	const QString& check_box_text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return ShowCheckBox(text,
		check_box_text,
		title, 
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_QUESTION),
		buttons,
		default_button,
		parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::ShowCheckBoxInformation(const QString& text,
	const QString& check_box_text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return ShowCheckBox(text,
		check_box_text,
		title, 
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_INFORMATION),
		buttons,
		default_button,
		parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::ShowCheckBox(const QString& text,
	const QString& check_box_text,
	const QString& title,
	bool enable_countdown,
	const QIcon& icon,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
    if (!parent) {
		parent = GetMainWindow();
    }
    if (parent != nullptr) {
        parent->setFocus();
    }
	QScopedPointer<MaskWidget> mask_widget(new MaskWidget(parent));
	XMessageBox box(title, text, parent, buttons, default_button, enable_countdown);
	box.SetIcon(icon);
	auto* check_box = new QCheckBox(&box);
	check_box->setStyleSheet(qTEXT("background: transparent;"));
	check_box->setText(check_box_text);
	box.AddWidget(check_box);
	if (box.exec() == -1) {
		return { QDialogButtonBox::Cancel, false };
	}
	const auto standard_button = box.StandardButton(box.ClickedButton());
	if (standard_button != default_button) {
		return { QDialogButtonBox::Yes, check_box->isChecked() };
	}
	return { QDialogButtonBox::Cancel, check_box->isChecked() };
}
