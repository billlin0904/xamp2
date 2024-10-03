#include <QCloseEvent>

#include <widget/xdialog.h>

#include <thememanager.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

#include <QWKWidgets/widgetwindowagent.h>

#include <widgetframe/windowbar.h>
#include <widgetframe/windowbutton.h>

#include <QLabel>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

XDialog::XDialog(QWidget* parent, bool modal)
    : QDialog(parent) {
    setModal(modal);
}

void XDialog::setContent(QWidget* content) {
    content_ = content;

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    setLayout(default_layout);

    default_layout->addWidget(content_, 1);
    default_layout->setContentsMargins(0, 0, 0, 0);

    // 重要! 避免出現setGeometry Unable to set geometry錯誤
    adjustSize();
	setFixedSize(size());
}

void XDialog::installWindowAgent() {
//    window_agent_ = new QWK::WidgetWindowAgent(this);
//    window_agent_->setup(this);
//
//    auto* title_label = new QLabel();
//    title_label->setAlignment(Qt::AlignCenter);
//    title_label->setObjectName("win-title-label"_str);
//
//#ifndef Q_OS_MAC
//    auto set_button_style = [](auto* button) {
//        const QColor color_hover_color("#dc3545"_str);
//
//        auto name = button->objectName();
//        button->setStyleSheet(qFormat(R"(
//                                         QPushButton#%1 {
//                                         border: none;
//                                         background-color: transparent;
//										 border-radius: 0px;
//                                         }
//
//										 QPushButton#%1:hover {
//										 background-color: %2;
//										 border-radius: 0px;
//                                         }
//                                         )").arg(name).arg(colorToString(color_hover_color)));
//        };
//
//    auto* icon_button = new QWK::WindowButton();
//    icon_button->setObjectName(QStringLiteral("icon-button"));
//    icon_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//    icon_button->setMinimumSize(QSize(32, 32));
//    icon_button->setIconSize(QSize(16, 16));
//    icon_button->setIconNormal(qTheme.applicationIcon());
//    set_button_style(icon_button);
//
//    auto* min_button = new QWK::WindowButton();
//    min_button->setObjectName("min-button"_str);
//    min_button->setProperty("system-button", true);
//    min_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//    min_button->setIconNormal(qTheme.fontIcon(Glyphs::ICON_MINIMIZE_WINDOW));
//    min_button->setIconSize(QSize(8, 8));
//    min_button->setMinimumSize(QSize(32, 32));
//    set_button_style(min_button);
//
//    auto* max_button = new QWK::WindowButton();
//    max_button->setCheckable(true);
//    max_button->setObjectName("max-button"_str);
//    max_button->setProperty("system-button", true);
//    max_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//    max_button->setIconNormal(qTheme.fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
//    max_button->setIconSize(QSize(8, 8));
//    max_button->setMinimumSize(QSize(32, 32));
//    set_button_style(max_button);
//
//    auto* close_button = new QWK::WindowButton();
//    close_button->setObjectName("close-button"_str);
//    close_button->setProperty("system-button", true);
//    close_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//    close_button->setIconNormal(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW));
//    close_button->setIconSize(QSize(8, 8));
//    close_button->setMinimumSize(QSize(32, 32));
//    set_button_style(close_button);
//
//#endif
//
//    auto* window_bar = new QWK::WindowBar();
//#ifndef Q_OS_MAC
//    window_bar->setIconButton(icon_button);
//    window_bar->setMinButton(min_button);
//    window_bar->setMaxButton(max_button);
//    window_bar->setCloseButton(close_button);
//#endif
//    window_bar->setTitleLabel(title_label);
//    window_bar->setHostWidget(this);
//
//    window_agent_->setTitleBar(window_bar);
//#ifndef Q_OS_MAC
//    window_agent_->setSystemButton(QWK::WindowAgentBase::WindowIcon, icon_button);
//    window_agent_->setSystemButton(QWK::WindowAgentBase::Minimize, min_button);
//    window_agent_->setSystemButton(QWK::WindowAgentBase::Maximize, max_button);
//    window_agent_->setSystemButton(QWK::WindowAgentBase::Close, close_button);
//#endif
//
//#ifdef Q_OS_MAC
//    window_agent_->setSystemButtonAreaCallback([](const QSize& size) {
//        static constexpr const int width = 75;
//        return QRect(QPoint(size.width() - width, 0), QSize(width, size.height())); //
//        });
//#endif
//
//    setMenuWidget(window_bar);
//
//
//#ifndef Q_OS_MAC
//    QObject::connect(window_bar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
//    QObject::connect(window_bar, &QWK::WindowBar::maximizeRequested, this, [this, max_button](bool max) {
//        if (max) {
//            showMaximized();
//        }
//        else {
//            showNormal();
//        }
//
//        // It's a Qt issue that if a QAbstractButton::clicked triggers a window's maximization,
//        // the button remains to be hovered until the mouse move. As a result, we need to
//        // manually send leave events to the button.
//        //emulateLeaveEvent(maxButton);
//        });
//    QObject::connect(window_bar, &QWK::WindowBar::closeRequested, this, &QWidget::close);
//#endif
//
//    window_agent_->setWindowAttribute("blur-effect"_str, "dark"_str);
}

void XDialog::onThemeChangedFinished(ThemeColor theme_color) {
}

void XDialog::setTitle(const QString& title) {
    setWindowTitle(title);
}

void XDialog::setIcon(const QIcon& icon) {
    setWindowIcon(icon);
}

void XDialog::setContentWidget(QWidget* content, bool no_moveable, bool disable_resize) {
    setContent(content);
}

void XDialog::closeEvent(QCloseEvent* event) {
    if (!content_) {
        return;
    }
    if (!content_->close()) {
        event->ignore();
        return;
    }
}
