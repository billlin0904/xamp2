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
	// 255 * 0.4 = 102
	setStyleSheet(qTEXT("background-color: rgba(0, 0, 0, 102);"));
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
	QMessageBox::StandardButton buttons,
	QMessageBox::StandardButton default_button)
	: XDialog(parent) {
	buttonBox_ = new QDialogButtonBox(this);
	buttonBox_->setStandardButtons(QDialogButtonBox::StandardButtons(static_cast<int>(buttons)));
	setDefaultButton(default_button);

	iconLabel_ = new QLabel(this);
	messageTextLabel_ = new QLabel(this);

	iconLabel_->setFixedSize(35, 35);
	iconLabel_->setScaledContents(true);
	iconLabel_->setStyleSheet(qTEXT("background: transparent;"));

	messageTextLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	messageTextLabel_->setObjectName(qTEXT("messageTextLabel"));
	messageTextLabel_->setOpenExternalLinks(true);
	messageTextLabel_->setFixedHeight(80);
	messageTextLabel_->setText(text);
	messageTextLabel_->setStyleSheet(qTEXT("background: transparent;"));

	auto* line = new QFrame(this);
	line->setFixedHeight(1);
	line->setFrameShape(QFrame::HLine);

	auto* client_widget = new QWidget(this);
	gridLayout_ = new QGridLayout(client_widget);
	gridLayout_->addWidget(iconLabel_, 0, 0, 2, 1, Qt::AlignTop);
	gridLayout_->addWidget(messageTextLabel_, 0, 1, 2, 1);
	gridLayout_->addWidget(line, gridLayout_->rowCount(), 0, 1, 2);
	gridLayout_->addWidget(buttonBox_, gridLayout_->rowCount(), 0, 1, gridLayout_->columnCount());
	gridLayout_->setSizeConstraint(QLayout::SetNoConstraint);
	gridLayout_->setHorizontalSpacing(10);
	gridLayout_->setVerticalSpacing(10);
	gridLayout_->setContentsMargins(10, 10, 10, 10);	

	QObject::connect(buttonBox_, &QDialogButtonBox::clicked, [this](auto* button) {
		onButtonClicked(button);
		});

	setContentWidget(client_widget);
	setTitle(title);
}

void XMessageBox::setText(const QString& text) {
	messageTextLabel_->setText(text);
}

void XMessageBox::onButtonClicked(QAbstractButton* button) {
	clickedButton_ = button;
	done(execReturnCode(button));
}

int XMessageBox::execReturnCode(QAbstractButton* button) {
	int nResult = buttonBox_->standardButton(button);
	return nResult;
}

void XMessageBox::setDefaultButton(QPushButton* button) {
	if (!buttonBox_->buttons().contains(button))
		return;

	defaultButton_ = button;
	button->setDefault(true);
	button->setFocus();
}

void XMessageBox::setDefaultButton(QMessageBox::StandardButton button) {
	setDefaultButton(buttonBox_->button(static_cast<QDialogButtonBox::StandardButton>(button)));
}

void XMessageBox::setIcon(const QIcon& icon) {
	iconLabel_->setPixmap(icon.pixmap(35, 35));
}

QAbstractButton* XMessageBox::clickedButton() const {
	return clickedButton_;
}

QMessageBox::StandardButton XMessageBox::standardButton(QAbstractButton* button) const {
	return static_cast<QMessageBox::StandardButton>(buttonBox_->standardButton(button));
}

void XMessageBox::addWidget(QWidget* widget) {
	//messageTextLabel_->hide();
	gridLayout_->addWidget(widget, 0, 1, 2, 1);
}

void XMessageBox::addButton(QMessageBox::StandardButton buttons) {
	buttonBox_->addButton(static_cast<QDialogButtonBox::StandardButton>(buttons));
}

QMessageBox::StandardButton XMessageBox::showButton(const QString& text,
	const QString& title,
	const QIcon& icon,
	QWidget* parent,
	QMessageBox::StandardButton buttons) {
	QScopedPointer<MaskWidget> mask_widget(new MaskWidget(parent));
	XMessageBox box(title, text, parent, buttons);
	box.setIcon(icon);
	if (box.exec() == -1)
		return QMessageBox::Cancel;	
	return box.standardButton(box.clickedButton());
}

QMessageBox::StandardButton XMessageBox::showInformation(const QString& text,
	const QString& title,
	QWidget* parent,
	QMessageBox::StandardButton buttons) {
	return showButton(text, title, qTheme.iconFromFont(0xF05A), parent, buttons);
}

QMessageBox::StandardButton XMessageBox::showError(const QString& text,
	const QString& title,
	QWidget* parent,
	QMessageBox::StandardButton buttons) {
	return showButton(text, title, qTheme.iconFromFont(0xF188), parent, buttons);
}

QMessageBox::StandardButton XMessageBox::showCheckBoxQuestion(const QString& text,
	const QString& check_box_text,
	const QString& title,
	QWidget* parent,
	QMessageBox::StandardButton buttons,
	QMessageBox::StandardButton default_button) {
	QScopedPointer<MaskWidget> mask_widget(new MaskWidget(parent));
	XMessageBox box(title, text, parent, buttons);
	box.setIcon(qTheme.iconFromFont(0xF05A));
	auto* check_box = new QCheckBox(&box);
	check_box->setStyleSheet(qTEXT("background: transparent;"));
	check_box->setText(check_box_text);
	box.addWidget(check_box);
	if (box.exec() == -1)
		return QMessageBox::Cancel;

	const auto standard_button = box.standardButton(box.clickedButton());
	if (standard_button == QMessageBox::Ok) {
		return check_box->isChecked() ? QMessageBox::Yes : QMessageBox::No;
	}
	return QMessageBox::Cancel;
}