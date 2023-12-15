#include <QCloseEvent>

#include <widget/xdialog.h>

#include <thememanager.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <FramelessHelper/Widgets/framelesswidgetshelper.h>

#include <QLabel>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

XDialog::XDialog(QWidget* parent, bool modal)
    : FramelessDialog(parent) {
    setModal(modal);
}

void XDialog::setContent(QWidget* content) {
    content_ = content;

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    setLayout(default_layout);

    title_frame_ = new QFrame();
    title_frame_->setObjectName(QString::fromUtf8("titleFrame"));
    title_frame_->setMinimumSize(QSize(0, kMaxTitleHeight));
    title_frame_->setFrameShape(QFrame::NoFrame);
    title_frame_->setFrameShadow(QFrame::Plain);

    auto f = font();
    f.setBold(true);
    f.setPointSize(qTheme.fontSize(kTitleFontSize));
    title_frame_label_ = new QLabel(title_frame_);
    title_frame_label_->setObjectName(QString::fromUtf8("titleFrameLabel"));
    QSizePolicy size_policy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    size_policy3.setHorizontalStretch(1);
    size_policy3.setVerticalStretch(0);
    size_policy3.setHeightForWidth(title_frame_label_->sizePolicy().hasHeightForWidth());
    title_frame_label_->setFont(f);
    title_frame_label_->setSizePolicy(size_policy3);
    title_frame_label_->setAlignment(Qt::AlignCenter);

    QString color;
    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        color = qTEXT("white");
        break;
    case ThemeColor::LIGHT_THEME:
        color = qTEXT("gray");
        break;
    }
    title_frame_label_->setStyleSheet(qSTR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
	color: %1;
    }
    )").arg(color));

    close_button_ = new QToolButton(title_frame_);
    close_button_->setObjectName(QString::fromUtf8("closeButton"));
    close_button_->setMinimumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    close_button_->setMaximumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    close_button_->setFocusPolicy(Qt::NoFocus);

    max_win_button_ = new QToolButton(title_frame_);
    max_win_button_->setObjectName(QString::fromUtf8("maxWinButton"));
    max_win_button_->setMinimumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    max_win_button_->setMaximumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    max_win_button_->setFocusPolicy(Qt::NoFocus);

    min_win_button_ = new QToolButton(title_frame_);
    min_win_button_->setObjectName(QString::fromUtf8("minWinButton"));
    min_win_button_->setMinimumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    min_win_button_->setMaximumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    min_win_button_->setFocusPolicy(Qt::NoFocus);
    min_win_button_->setPopupMode(QToolButton::InstantPopup);

    icon_ = new QToolButton(title_frame_);
    icon_->setObjectName(QString::fromUtf8("minWinButton"));
    icon_->setMinimumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    icon_->setMaximumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
    icon_->setIconSize(QSize(kMaxTitleIcon, kMaxTitleIcon));
    icon_->setFocusPolicy(Qt::NoFocus);
    icon_->setStyleSheet(qTEXT("background: transparent; border: none;"));
    icon_->hide();

    auto* horizontal_spacer = new QSpacerItem(kMaxTitleHeight, kMaxTitleIcon, QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* horizontal_layout = new QHBoxLayout(title_frame_);
    horizontal_layout->addWidget(icon_);
    horizontal_layout->addItem(horizontal_spacer);
    horizontal_layout->addWidget(title_frame_label_);
    horizontal_layout->addWidget(min_win_button_);
    horizontal_layout->addWidget(max_win_button_);
    horizontal_layout->addWidget(close_button_);

    horizontal_layout->setSpacing(0);
    horizontal_layout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontal_layout->setContentsMargins(0, 0, 0, 0);

    default_layout->addWidget(title_frame_, 0);
    default_layout->addWidget(content_, 1);
    default_layout->setContentsMargins(0, 0, 0, 0);

    qTheme.setTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);

    max_win_button_->setDisabled(true);
    min_win_button_->setDisabled(true);
    
    max_win_button_->hide();
    min_win_button_->hide();
    // todo: close_button_ hide的時候會顯示會有問題.

    (void)QObject::connect(close_button_, &QToolButton::clicked, [this]() {
        close();
        });

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        this,
        &XDialog::onCurrentThemeChanged);

    FramelessWidgetsHelper::get(this)->setTitleBarWidget(title_frame_);
    FramelessWidgetsHelper::get(this)->setSystemButton(min_win_button_, Global::SystemButtonType::Minimize);
    FramelessWidgetsHelper::get(this)->setSystemButton(max_win_button_, Global::SystemButtonType::Maximize);
    FramelessWidgetsHelper::get(this)->setSystemButton(close_button_, Global::SystemButtonType::Close);
    
    // 1. 如果使用waitForReady就會一直接收到message,
    // 會導致無法訊息一直出現並會遞迴.
    // 2. 無法將視窗置中.
    //FramelessWidgetsHelper::get(this)->waitForReady();

    // 重要! 避免出現setGeometry Unable to set geometry錯誤
    adjustSize();
}

void XDialog::onCurrentThemeChanged(ThemeColor theme_color) {
    qTheme.setTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);
}

void XDialog::setTitle(const QString& title) const {
    title_frame_label_->setText(title);
}

void XDialog::setContentWidget(QWidget* content, bool no_moveable, bool disable_resize) {
    setContent(content);
    FramelessWidgetsHelper::get(this)->setWindowFixedSize(disable_resize);
    if (no_moveable) {
        FramelessWidgetsHelper::get(this)->setHitTestVisible(title_frame_);
    }
}

void XDialog::setIcon(const QIcon& icon) const {
    icon_->setIcon(icon);
    icon_->setHidden(false);
}

void XDialog::showEvent(QShowEvent* event) {
    /*auto* opacity_effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacity_effect);
    auto* opacity_animation = new QPropertyAnimation(opacity_effect, "opacity", this);
    opacity_animation->setStartValue(0);
    opacity_animation->setEndValue(1);
    opacity_animation->setDuration(200);
    opacity_animation->setEasingCurve(QEasingCurve::OutCubic);
    (void)QObject::connect(opacity_animation,
        &QPropertyAnimation::finished,
        opacity_effect,
        &QGraphicsOpacityEffect::deleteLater);
    opacity_animation->start();

    setAttribute(Qt::WA_Mapped);*/
    QDialog::showEvent(event);
}

void XDialog::closeEvent(QCloseEvent* event) {
    if (!content_->close()) {
        event->ignore();
        return;
    }
	FramelessDialog::closeEvent(event);
}
