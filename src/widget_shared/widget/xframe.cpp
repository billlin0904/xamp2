#include <widget/str_utilts.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QToolButton>
#include <widget/xframe.h>

#include <thememanager.h>

XFrame::XFrame(QWidget* parent)
    : QWidget(parent) {
    setObjectName(qTEXT("XFrame"));
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(qSTR(
        R"(
			QWidget#XFrame {
				border: none;
				border-radius: 8px;
			}
        )"));
}

void XFrame::SetTitle(const QString& title) const {
    title_frame_label_->setText(title);
}

void XFrame::SetIcon(const QIcon& icon) const {
    icon_->setIcon(icon);
    icon_->setHidden(false);
}

void XFrame::OnCurrentThemeChanged(ThemeColor theme_color) {
    qTheme.SetTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);
}

void XFrame::SetContentWidget(QWidget* content) {
    content_ = content;

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(5, 0, 5, 5);
    setLayout(default_layout);

    QFrame* title_frame = nullptr;
    if (!qTheme.UseNativeWindow()) {
        title_frame = new QFrame(this);
    } else {
        title_frame = new QFrame();
    }

    title_frame->setObjectName(QString::fromUtf8("titleFrame"));
    title_frame->setMinimumSize(QSize(0, 24));
    title_frame->setFrameShape(QFrame::NoFrame);
    title_frame->setFrameShadow(QFrame::Plain);

    auto f = font();
    f.setBold(true);
    f.setPointSize(qTheme.GetFontSize(10));
    title_frame_label_ = new QLabel(title_frame);
    title_frame_label_->setObjectName(QString::fromUtf8("titleFrameLabel"));
    QSizePolicy size_policy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    size_policy3.setHorizontalStretch(1);
    size_policy3.setVerticalStretch(0);
    size_policy3.setHeightForWidth(title_frame_label_->sizePolicy().hasHeightForWidth());
    title_frame_label_->setFont(f);
    title_frame_label_->setSizePolicy(size_policy3);
    title_frame_label_->setAlignment(Qt::AlignCenter);
    title_frame_label_->setStyleSheet(qSTR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

    close_button_ = new QToolButton(title_frame);
    close_button_->setObjectName(QString::fromUtf8("closeButton"));
    close_button_->setMinimumSize(QSize(24, 24));
    close_button_->setMaximumSize(QSize(24, 24));
    close_button_->setFocusPolicy(Qt::NoFocus);

    max_win_button_ = new QToolButton(title_frame);
    max_win_button_->setObjectName(QString::fromUtf8("maxWinButton"));
    max_win_button_->setMinimumSize(QSize(24, 24));
    max_win_button_->setMaximumSize(QSize(24, 24));
    max_win_button_->setFocusPolicy(Qt::NoFocus);

    min_win_button_ = new QToolButton(title_frame);
    min_win_button_->setObjectName(QString::fromUtf8("minWinButton"));
    min_win_button_->setMinimumSize(QSize(24, 24));
    min_win_button_->setMaximumSize(QSize(24, 24));
    min_win_button_->setFocusPolicy(Qt::NoFocus);
    min_win_button_->setPopupMode(QToolButton::InstantPopup);

    icon_ = new QToolButton(title_frame);
    icon_->setObjectName(QString::fromUtf8("minWinButton"));
    icon_->setMinimumSize(QSize(24, 24));
    icon_->setMaximumSize(QSize(24, 24));
    icon_->setFocusPolicy(Qt::NoFocus);
    icon_->setStyleSheet(qTEXT("background: transparent; border: none;"));
    icon_->hide();

    auto* horizontal_spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* horizontal_layout = new QHBoxLayout(title_frame);
    horizontal_layout->addWidget(icon_);
    horizontal_layout->addItem(horizontal_spacer);
    horizontal_layout->addWidget(title_frame_label_);
    horizontal_layout->addWidget(min_win_button_);
    horizontal_layout->addWidget(max_win_button_);
    horizontal_layout->addWidget(close_button_);

    horizontal_layout->setSpacing(0);
    horizontal_layout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontal_layout->setContentsMargins(0, 0, 0, 0);

    if (!qTheme.UseNativeWindow()) {
        default_layout->addWidget(title_frame, 0);
    }

    default_layout->addWidget(content_, 1);
    default_layout->setContentsMargins(0, 0, 0, 0);

    qTheme.SetTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);

    max_win_button_->setDisabled(true);
    min_win_button_->setDisabled(true);

    max_win_button_->hide();
    min_win_button_->hide();

    (void)QObject::connect(close_button_, &QToolButton::pressed, [this]() {
        QWidget::close();
        emit CloseFrame();
        });    

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        this,
        &XFrame::OnCurrentThemeChanged);
}

