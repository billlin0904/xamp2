#include <widget/xprogressdialog.h>
#include <widget/maskwidget.h>
#include <QGridLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

#include <widget/util/ui_util.h>

XProgressDialog::XProgressDialog(const QString& title,
                                 const QString& cancel_text,
                                 int minimum, 
								 int maximum,
                                 QWidget* parent)
	: XDialog(parent, true) {
	mask_widget_.reset(new MaskWidget(parent));

	message_text_label_ = new QLabel(this);
	sub_text_label_ = new QLabel(this);
	progress_bar_ = new QProgressBar(this);
	default_button_ = new QPushButton(this);

	auto* line = new QFrame(this);
	line->setFixedHeight(1);
	line->setFrameShape(QFrame::HLine);

	progress_bar_->setFont(QFont("FormatFont"_str));
	progress_bar_->setFixedHeight(15);
	progress_bar_->setRange(minimum, maximum);

	message_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	message_text_label_->setObjectName("messageTextLabel"_str);
	message_text_label_->setOpenExternalLinks(false);
	message_text_label_->setFixedHeight(20);
	message_text_label_->setStyleSheet("background: transparent;"_str);

	sub_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	sub_text_label_->setFixedHeight(20);
	sub_text_label_->setStyleSheet("background: transparent;"_str);

	default_button_->setText(cancel_text);

	auto* spacer_item = new QSpacerItem(10, 17, QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto *horizontal_layout = new QHBoxLayout();
	horizontal_layout->setSpacing(0);
	horizontal_layout->addItem(spacer_item);
	horizontal_layout->addWidget(sub_text_label_);
	horizontal_layout->addWidget(default_button_);

	auto* client_widget = new QWidget(this);
	layout_ = new QVBoxLayout(client_widget);
	layout_->addWidget(message_text_label_);
	layout_->addWidget(progress_bar_);
	layout_->addLayout(horizontal_layout);
	layout_->setSizeConstraint(QLayout::SetNoConstraint);
	layout_->setSpacing(5);
	layout_->setContentsMargins(10, 10, 10, 10);

	setContentWidget(client_widget);
	setTitle(title);

	if (parent) {
		max_width_ = parent->width() * 0.8;
		setMaximumWidth(parent->width() - 100);
		setMinimumWidth(parent->width() - 100);
	}	

	size_ = size();

	(void)QObject::connect(default_button_,
		&QAbstractButton::pressed,
		[this]() {
		progress_bar_->reset();
		close();
		emit cancelRequested();
		});

	centerParent(this);
}

XProgressDialog::~XProgressDialog() = default;

void XProgressDialog::setRange(int minimum, int maximum) {
	progress_bar_->setRange(minimum, maximum);
}

void XProgressDialog::setSubValue(int total, int current) {
	sub_text_label_->setText(qFormat("%1 / %2").arg(current).arg(total));
}

int XProgressDialog::value() const {
	return progress_bar_->value();
}

void XProgressDialog::setValue(int value) {
	if (size_ != size()) {
		centerParent(this);
		size_ = size();
	}
	progress_bar_->setValue(value);
}

void XProgressDialog::setLabelText(const QString& text) {
	const QFontMetrics metrics(font());
	message_text_label_->setText(metrics.elidedText(text, Qt::ElideRight, max_width_));
	centerParent(this);
}

bool XProgressDialog::wasCanceled() const {
	return default_button_->isChecked();
}