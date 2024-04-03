#include <widget/xmessagebox.h>
#include <widget/widget_shared.h>
#include <widget/xmainwindow.h>
#include <widget/util/ui_utilts.h>
#include <widget/util/str_utilts.h>

#include <base/exception.h>

#include <QLabel>
#include <QApplication>
#include <QCheckBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QScopedPointer>

XMessageBox::XMessageBox(const QString& title,
                         const QString& text,
                         QWidget* parent,
                         const QFlags<QDialogButtonBox::StandardButton> buttons,
                         const QDialogButtonBox::StandardButton default_button,
                         const bool enable_countdown)
	: XDialog(parent) {
	mask_widget_.reset(new MaskWidget(parent));

	enable_countdown_ = enable_countdown;

	button_box_ = new QDialogButtonBox(this);
	button_box_->setStandardButtons(QDialogButtonBox::StandardButtons(buttons));
	setDefaultButton(default_button);

	icon_label_ = new QLabel(this);
	message_text_label_ = new QLabel(this);

	icon_label_->setFixedSize(40, 40);
	icon_label_->setScaledContents(true);
	icon_label_->setStyleSheet(qTEXT("background: transparent;"));

	message_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	message_text_label_->setObjectName(qTEXT("messageTextLabel"));
	message_text_label_->setOpenExternalLinks(true);
	//message_text_label_->setFixedHeight(80);
	//message_text_label_->setFixedWidth(80);
	message_text_label_->setText(text);

	QFont f;
	f.setPointSize(qTheme.fontSize(10));
	message_text_label_->setFont(f);
	message_text_label_->setStyleSheet(qTEXT("background: transparent;"));

	auto* line = new QFrame(this);
	line->setFixedHeight(1);
	line->setFrameShape(QFrame::HLine);

	auto* client_widget = new QWidget(this);
	grid_layout_ = new QGridLayout(client_widget);
	grid_layout_->addWidget(icon_label_, 0, 0, 2, 1, Qt::AlignTop);
	grid_layout_->addWidget(message_text_label_, 0, 1, 3, 1);
	grid_layout_->addWidget(line, grid_layout_->rowCount(), 0, 1, grid_layout_->columnCount());
	grid_layout_->addWidget(button_box_, grid_layout_->rowCount(), 0, 1, grid_layout_->columnCount());
	grid_layout_->setSizeConstraint(QLayout::SetNoConstraint);
	grid_layout_->setHorizontalSpacing(0);
	grid_layout_->setVerticalSpacing(10);
	grid_layout_->setContentsMargins(10, 10, 10, 10);	

	(void)QObject::connect(button_box_, &QDialogButtonBox::clicked, [this](auto* button) {
		onButtonClicked(button);
		});

	default_button_text_ = defaultButton()->text();

	setContentWidget(client_widget);
	setTitle(title);
	XDialog::setIcon(qTheme.applicationIcon());

	(void)QObject::connect(&timer_, &QTimer::timeout, this, &XMessageBox::onUpdate);
	timer_.setInterval(1000);
	centerParent(this);

	const auto metrics = defaultButton()->fontMetrics();
	setMinimumSize(QSize(metrics.horizontalAdvance(default_button_text_) * 1.5, 100));
	adjustSize();
}

XMessageBox::~XMessageBox() = default;

void XMessageBox::setTextFont(const QFont& font) {
	message_text_label_->setFont(font);
}

void XMessageBox::setText(const QString& text) {
	message_text_label_->setText(text);
	const auto metrics = defaultButton()->fontMetrics();
	setMinimumSize(QSize(metrics.horizontalAdvance(text) * 1.5, 100));
	adjustSize();
	centerParent(this);
}

void XMessageBox::onButtonClicked(QAbstractButton* button) {
	clicked_button_ = button;
	done(execReturnCode(button));
}

int XMessageBox::execReturnCode(QAbstractButton* button) {
	return button_box_->standardButton(button);
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
	if (!enable_countdown_) {
		return;
	}
	onUpdate();
	timer_.start();
}

void XMessageBox::onUpdate() {
	if (--timeout_ >= 1) {
		defaultButton()->setText(default_button_text_ + qSTR(" (%1)").arg(timeout_));
	} else {
		timer_.stop();
		defaultButton()->animateClick();
	}
}

void XMessageBox::showBug(const Exception& exception,
	const QString& title,
	QWidget* parent) {
	XMessageBox box(title, QString::fromStdString(exception.GetStackTrace()), 
		parent, 
		QDialogButtonBox::Ok,
		QDialogButtonBox::Ok,
		false);
	box.setIcon(qTheme.fontIcon(0xF188));
	box.setTextFont(QFont(qTEXT("DebugFont")));
	box.exec();
}

QDialogButtonBox::StandardButton XMessageBox::showYesOrNo(const QString& text,
	const QString& title,
	bool enable_countdown,
	QWidget* parent) {
	return showWarning(text,
		title,
		enable_countdown,
		QDialogButtonBox::No | QDialogButtonBox::Yes,
		QDialogButtonBox::No,
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::showButton(const QString& text,
	const QString& title,
	bool enable_countdown,
	const QIcon& icon,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
    if (!parent) {
		parent = getMainWindow();
	}
	if (parent != nullptr) {
		parent->setFocus();
	}
	XMessageBox box(title, text, parent, buttons, default_button, enable_countdown);
	// Note: Don't call centerParent(), centerDesktop()
	box.setIcon(icon);
	if (box.exec() == -1)
		return QDialogButtonBox::Cancel;
	return box.standardButton(box.clickedButton());
}

QDialogButtonBox::StandardButton XMessageBox::showInformation(const QString& text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showButton(text, 
		title, 
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_INFORMATION),
		buttons,
		default_button,
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::showError(const QString& text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showButton(text, 
		title,
		enable_countdown, 
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_ERROR),
		buttons, 
		default_button, 
		parent);
}

QDialogButtonBox::StandardButton XMessageBox::showWarning(const QString& text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showButton(text,
		title,
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_WARNING),
		buttons,
		default_button,
		parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::showCheckBoxQuestion(const QString& text,
	const QString& check_box_text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showCheckBox(text,
		check_box_text,
		title, 
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_QUESTION),
		buttons,
		default_button,
		parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::showCheckBoxInformation(const QString& text,
	const QString& check_box_text,
	const QString& title,
	bool enable_countdown,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
	return showCheckBox(text,
		check_box_text,
		title, 
		enable_countdown,
		qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_INFORMATION),
		buttons,
		default_button,
		parent);
}

std::tuple<QDialogButtonBox::StandardButton, bool> XMessageBox::showCheckBox(const QString& text,
	const QString& check_box_text,
	const QString& title,
	bool enable_countdown,
	const QIcon& icon,
	QFlags<QDialogButtonBox::StandardButton> buttons,
	QDialogButtonBox::StandardButton default_button,
	QWidget* parent) {
    if (!parent) {
		parent = getMainWindow();
    }
    if (parent != nullptr) {
        parent->setFocus();
    }
	XMessageBox box(title, text, parent, buttons, default_button, enable_countdown);
	box.setIcon(icon);
	auto* check_box = new QCheckBox(&box);
	check_box->setStyleSheet(qTEXT("background: transparent;"));
	check_box->setText(check_box_text);
	box.addWidget(check_box);
	if (box.exec() == -1) {
		return { QDialogButtonBox::Cancel, false };
	}
	const auto standard_button = box.standardButton(box.clickedButton());
	if (standard_button != default_button) {
		return { QDialogButtonBox::Yes, check_box->isChecked() };
	}
	return { QDialogButtonBox::Cancel, check_box->isChecked() };
}
