#include <widget/xmessage.h>

#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QStyleOption>
#include <QPainter>

#include <thememanager.h>

constexpr int kMessageItemMargin = 20;

constexpr int kIconMargin = 12;
constexpr int kLeftMargin = 64;
constexpr int kTopMargin = 10;
constexpr int kMinWidth = 400;
constexpr int kMinHeight = 70;

XMessage::XMessage(QWidget* parent)
    : QWidget(parent) {
    width_ = parent->width();
    duration_ms_ = 3000;
    items_.reserve(50);
}

XMessage::~XMessage() = default;

void XMessage::Push(MessageTypes type, const QString& content) {
    int height = 0;
    std::for_each(items_.begin(), items_.end(), [&height](const auto* item) mutable {
        height += (kMessageItemMargin + item->height());
        });

    auto *item = new XMessageItem(static_cast<QWidget*>(parent()), type, content);
    (void)QObject::connect(item, &XMessageItem::ItemReadyRemoved, this, &XMessage::AdjustItemPosition);
    (void)QObject::connect(item, &XMessageItem::ItemRemoved, this, &XMessage::RemoveItem);
    item->SetDuration(duration_ms_);
    height += kMessageItemMargin;
    item->move(QPoint((width_ - item->width()) / 2, height));
    items_.push_back(item);
    item->Show();
}

void XMessage::SetDuration(int duration_ms) {
    if (duration_ms < 0)
        duration_ms = 0;
    duration_ms_ = duration_ms;
}

void XMessage::AdjustItemPosition(XMessageItem* item) {
    item->Close();
}

void XMessage::RemoveItem(XMessageItem* item) {
    items_.removeOne(item);

    auto height = kMessageItemMargin;
    std::for_each(items_.begin(), items_.end(), [&](XMessageItem* item) {
        auto* animation = new QPropertyAnimation(item, "geometry", this);
        animation->setDuration(300);
        animation->setStartValue(QRect(item->pos().x(),
            item->pos().y(),
            item->width(),
            item->height()));
        animation->setEndValue(QRect(item->pos().x(),
            height,
            item->width(),
            item->height()));

        animation->start(QAbstractAnimation::DeletionPolicy::DeleteWhenStopped);
        height += (kMessageItemMargin + item->height());
        });
}

XMessageItem::XMessageItem(QWidget* parent,
    MessageTypes type,
    const QString& content)
    : QWidget(parent) {
    duration_ms_ = 3000;
    setObjectName(QStringLiteral("messageItem"));
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(qSTR(
        R"(
			QWidget#messageItem {
				border: none;
				border-radius: 8px;
			}
        )"
    ));

    auto *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(20);
    effect->setColor(Qt::black);
    effect->setOffset(0, 0);
    setGraphicsEffect(effect);

    icon_ = new QLabel(this);
    switch (type) {
    case MSG_SUCCESS:
        icon_->setPixmap(qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_SUCCESS).pixmap(35, 35));
        break;
    case MSG_WARNING:
        icon_->setPixmap(qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_WARNING).pixmap(35, 35));
        break;   
    case MSG_ERROR:
        icon_->setPixmap(qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_ERROR).pixmap(35, 35));
        break;
    case MSG_INFORMATION:
    default:
        icon_->setPixmap(qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_INFORMATION).pixmap(35, 35));
        break;
    }

    content_ = new QLabel(this);
    content_->setText(content);
    content_->adjustSize();
    width_ = content_->width() + kLeftMargin * 2;
    height_ = content_->height() + kTopMargin * 2;

    if (width_ < kMinWidth)
        width_ = kMinWidth;
    if (height_ < kMinHeight) 
        height_ = kMinHeight;

    resize(width_, height_);

    content_->move(kLeftMargin, (height_ - content_->height()) / 2);
    icon_->move(kIconMargin, (height_ - content_->height()) / 2);

    (void)QObject::connect(&timer_, &QTimer::timeout, this, [&]() {
        timer_.stop();
        emit ItemReadyRemoved(this);
        });

    icon_->setStyleSheet(qTEXT("background-color: transparent"));
    content_->setStyleSheet(qTEXT("background-color: transparent"));

    hide();
}

XMessageItem::~XMessageItem() = default;

void XMessageItem::Show() {
    show();
    if (duration_ms_ > 0)
        timer_.start(duration_ms_);
    AppearAnimation();
}

void XMessageItem::Close() {
    DisappearAnimation();
}

void XMessageItem::SetDuration(const int duration_ms) {
    duration_ms_ = duration_ms;
}

void XMessageItem::AppearAnimation() {
    auto* animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(20);
    animation->setStartValue(QRect(pos().x(), pos().y() - kMessageItemMargin, width_, height_));
    animation->setEndValue(QRect(pos().x(), pos().y(), width_, height_));
    animation->start(QAbstractAnimation::DeletionPolicy::DeleteWhenStopped);
}

void XMessageItem::DisappearAnimation() {
    auto* opacity_animation = new QGraphicsOpacityEffect(this);
    opacity_animation->setOpacity(1);
    setGraphicsEffect(opacity_animation);

    auto* property_animation = new QPropertyAnimation(opacity_animation, "opacity");
    property_animation->setDuration(500);
    property_animation->setStartValue(1);
    property_animation->setEndValue(0);

    property_animation->start(QAbstractAnimation::DeletionPolicy::DeleteWhenStopped);

    connect(property_animation, &QPropertyAnimation::finished, this, [&]() {
        emit ItemRemoved(this);
        deleteLater();
        });
}

void XMessageItem::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}
