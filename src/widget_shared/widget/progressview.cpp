#include <widget/progressview.h>

#include <widget/xprogressdialog.h>

#include <QGridLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

#include <widget/ui_utilts.h>

ProgressView::ProgressView(QWidget* parent,
	const QString& cancel_text,
	int minimum,
	int maximum)
	: QFrame(parent) {
	//setObjectName(qTEXT("progressView"));
	//setFrameStyle(QFrame::StyledPanel);

	message_text_label_ = new QLabel(this);
	sub_text_label_ = new QLabel(this);
	progress_bar_ = new QProgressBar(this);
	default_button_ = new QPushButton(this);

	/*auto* line = new QFrame(this);
	line->setFixedHeight(1);
	line->setFrameShape(QFrame::HLine);*/

	progress_bar_->setFont(QFont(qTEXT("FormatFont")));
	progress_bar_->setFixedHeight(15);
	progress_bar_->setRange(minimum, maximum);

	message_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	message_text_label_->setObjectName(qTEXT("messageTextLabel"));
	message_text_label_->setOpenExternalLinks(false);
	message_text_label_->setFixedHeight(20);
	//message_text_label_->setStyleSheet(qTEXT("background: transparent;"));

	sub_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	sub_text_label_->setFixedHeight(20);
	//sub_text_label_->setStyleSheet(qTEXT("background: transparent;"));

	default_button_->setText(cancel_text);

	auto* spacer_item = new QSpacerItem(10, 17, QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto* horizontal_layout = new QHBoxLayout();
	horizontal_layout->setSpacing(0);
	horizontal_layout->addItem(spacer_item);
	horizontal_layout->addWidget(sub_text_label_);
	horizontal_layout->addWidget(default_button_);

	//auto* client_widget = new QWidget(this);
	//layout_ = new QVBoxLayout(client_widget);
	layout_ = new QVBoxLayout(this);
	layout_->addWidget(message_text_label_);
	layout_->addWidget(progress_bar_);
	layout_->addLayout(horizontal_layout);
	layout_->setSizeConstraint(QLayout::SetNoConstraint);
	layout_->setSpacing(5);
	layout_->setContentsMargins(10, 10, 10, 10);
	setLayout(layout_);

	//setContentWidget(client_widget);
	//setTitle(title);

	/*if (parent) {
		max_width_ = parent->width() * 0.8;
		setMaximumWidth(parent->width() - 100);
		setMinimumWidth(parent->width() - 100);
	}*/

	size_ = size();

	/*(void)QObject::connect(default_button_,
		&QAbstractButton::pressed,
		[this]() {
			progress_bar_->reset();
			close();
			emit cancelRequested();
		});*/

	//centerParent(this);

	setLineWidth(5);
	//setStyleSheet(qTEXT("background: white;"));
}

ProgressView::~ProgressView() {
}

void ProgressView::setRange(int minimum, int maximum) {
	progress_bar_->setRange(minimum, maximum);
}

void ProgressView::setSubValue(int total, int current) {
	sub_text_label_->setText(qSTR("%1 / %2").arg(current).arg(total));
}

int ProgressView::value() const {
	return progress_bar_->value();
}

void ProgressView::setValue(int value) {
	if (size_ != size()) {
		centerParent(this);
		size_ = size();
	}
	progress_bar_->setValue(value);
}

void ProgressView::setLabelText(const QString& text) {
	const QFontMetrics metrics(font());
	message_text_label_->setText(metrics.elidedText(text, Qt::ElideRight, max_width_));
	centerParent(this);
}

bool ProgressView::wasCanceled() const {
	return default_button_->isChecked();
}
