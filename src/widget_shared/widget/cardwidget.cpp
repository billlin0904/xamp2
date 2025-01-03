#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

#include <widget/util/str_util.h>
#include <widget/flowlayout.h>
#include <widget/cardwidget.h>

IconWidget::IconWidget(const QIcon& icon, QWidget* parent)
	: QWidget(parent)
	, icon_(icon) {
}

void IconWidget::setIcon(const QIcon& icon) {
	icon_ = icon;
	update();
}

void IconWidget::paintEvent(QPaintEvent* /*event*/) {
    if (icon_.isNull()) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    auto pixmap = icon_.pixmap(rect().size());
    painter.drawPixmap(rect(), pixmap);
}

CardWidget::CardWidget(const QIcon& icon, const QString& title, const QString& content, const QString& routeKey, int index, QWidget* parent)
    : QFrame(parent) {
    index_ = index;
    route_key_ = routeKey;

    icon_widget_ = new IconWidget(icon, this);
    title_label_ = new QLabel(title, this);
    content_label_ = new QLabel(/*TextWrap::wrap(content, 45, false).first*/content, this);

    h_box_layout_ = new QHBoxLayout(this);
    v_box_layout_ = new QVBoxLayout();

    setFixedSize(360, 90);
    icon_widget_->setFixedSize(48, 48);

    h_box_layout_->setSpacing(28);
    h_box_layout_->setContentsMargins(20, 0, 0, 0);

    v_box_layout_->setSpacing(2);
    v_box_layout_->setContentsMargins(0, 0, 0, 0);
    v_box_layout_->setAlignment(Qt::AlignVCenter);

    h_box_layout_->setAlignment(Qt::AlignVCenter);
    h_box_layout_->addWidget(icon_widget_);
    h_box_layout_->addLayout(v_box_layout_);
    v_box_layout_->addStretch(1);
    v_box_layout_->addWidget(title_label_);
    v_box_layout_->addWidget(content_label_);
    v_box_layout_->addStretch(1);

    title_label_->setObjectName("titleLabel");
    content_label_->setObjectName("contentLabel");
}

void CardWidget::mouseReleaseEvent(QMouseEvent* event) {
	
}

CardView::CardView(const QString& title, QWidget* parent)
	: QWidget(parent) {
    title_label_ = new QLabel(title, this);
    v_box_layout_ = new QVBoxLayout(this);
    flow_layout_ = new FlowLayout();

    v_box_layout_->setContentsMargins(36, 0, 36, 0);
    v_box_layout_->setSpacing(0);
    flow_layout_->setContentsMargins(0, 0, 0, 0);
    flow_layout_->setHorizontalSpacing(12);
    flow_layout_->setVerticalSpacing(12);

    v_box_layout_->addWidget(title_label_);
    v_box_layout_->addLayout(flow_layout_, 1);

    title_label_->setObjectName("viewTitleLabel");
    setStyleSheet("background-color: transparent; border: none;"_str);
}

void CardView::addCard(const QIcon& icon, const QString& title, const QString& content, const QString& routeKey, int index) {
	auto* card = new CardWidget(icon, title, content, routeKey, index, this);
    flow_layout_->addWidget(card);
}

SettingCard::SettingCard(const QIcon& icon, const QString& title, const QString& content, QWidget* parent)
	: QFrame(parent)
	, icon_(icon) {
    icon_label_ = new IconWidget(icon, this);
    title_label_ = new QLabel(title, this);
    content_label_ = new QLabel(content, this);
    h_box_layout_ = new QHBoxLayout();
    v_box_layout_ = new QVBoxLayout();

    if (content.isEmpty()) {
        content_label_->hide();
    }

    h_box_layout_->setSpacing(0);
    h_box_layout_->setContentsMargins(16, 0, 0, 0);
    h_box_layout_->setAlignment(Qt::AlignVCenter);
    v_box_layout_->setSpacing(0);
    v_box_layout_->setContentsMargins(0, 0, 0, 0);
    v_box_layout_->setAlignment(Qt::AlignVCenter);

    h_box_layout_->addWidget(icon_label_, 0, Qt::AlignLeft);
    h_box_layout_->addSpacing(16);

    h_box_layout_->addLayout(v_box_layout_);
    v_box_layout_->addWidget(title_label_, 0, Qt::AlignLeft);
    v_box_layout_->addWidget(content_label_, 0, Qt::AlignLeft);

    h_box_layout_->addSpacing(16);
    h_box_layout_->addStretch(1);

    setLayout(h_box_layout_);

    content_label_->setObjectName("contentLabel");
}

void SettingCard::setTitle(const QString& title) {
    title_label_->setText(title);
}

void SettingCard::setContent(const QString& content) {
    content_label_->setText(content);
    content_label_->setVisible(!content.isEmpty());
}

QIcon SettingCard::icon() const {
    return icon_;
}

void SettingCard::setValue(const QVariant&) {
	
}