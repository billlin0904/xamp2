#include <QGridLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <widget/xprogressdialog.h>

XProgressDialog::XProgressDialog(const QString& title,
	const QString& cancel_text,
	int minimum, int maximum,
	QWidget* parent)
	: XDialog(parent) {
	message_text_label_ = new QLabel(this);
	progress_bar_ = new QProgressBar(this);
	default_button_ = new QPushButton(this);

	auto* line = new QFrame(this);
	line->setFixedHeight(1);
	line->setFrameShape(QFrame::HLine);

	progress_bar_->setFont(QFont(qTEXT("FormatFont")));

	message_text_label_->setAlignment(Qt::AlignCenter);
	message_text_label_->setObjectName(qTEXT("messageTextLabel"));
	message_text_label_->setOpenExternalLinks(true);
	message_text_label_->setStyleSheet(qTEXT("background: transparent;"));

	default_button_->setText(cancel_text);

	auto* spacer_item = new QSpacerItem(10, 17, QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto *horizontal_layout = new QHBoxLayout();
	horizontal_layout->setSpacing(0);
	horizontal_layout->addItem(spacer_item);
	horizontal_layout->addWidget(default_button_);

	auto* client_widget = new QWidget(this);
	layout_ = new QVBoxLayout(client_widget);
	layout_->addWidget(message_text_label_);
	layout_->addWidget(progress_bar_);
	layout_->addWidget(line);
	layout_->addLayout(horizontal_layout);
	layout_->setSizeConstraint(QLayout::SetNoConstraint);
	layout_->setSpacing(5);
	layout_->setContentsMargins(10, 10, 10, 10);

	setContentWidget(client_widget);
	setTitle(title);
}

void XProgressDialog::setRange(int minimum, int maximum) {
	progress_bar_->setRange(minimum, maximum);
}

void XProgressDialog::setValue(int value) {
	progress_bar_->setValue(value);
}

void XProgressDialog::setLabelText(const QString& text) {
	message_text_label_->setText(text);
}

bool XProgressDialog::wasCanceled() const {
	return default_button_->isChecked();
}