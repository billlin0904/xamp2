#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QToolButton>
#include <widget/xframe.h>

#include "str_utilts.h"
#include "thememanager.h"

XFrame::XFrame(QWidget* parent)
    : QFrame(parent) {
}

void XFrame::setContentWidget(QWidget* content) {
    content_widget_ = content;

    auto* default_layout = new QGridLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(0, 0, 0, 20);
    setLayout(default_layout);

    auto* titleFrame = new QFrame(this);
    titleFrame->setObjectName(QString::fromUtf8("titleFrame"));
    titleFrame->setMinimumSize(QSize(0, 24));
    titleFrame->setFrameShape(QFrame::NoFrame);
    titleFrame->setFrameShadow(QFrame::Plain);
    titleFrame->setStyleSheet(Q_UTF8("QFrame#titleFrame { border: none; }"));

    auto* titleFrameLabel = new QLabel(titleFrame);
    titleFrameLabel->setObjectName(QString::fromUtf8("titleFrameLabel"));
    QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy3.setHorizontalStretch(1);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(titleFrameLabel->sizePolicy().hasHeightForWidth());
    titleFrameLabel->setSizePolicy(sizePolicy3);

    auto* closeButton = new QToolButton(titleFrame);
    closeButton->setObjectName(QString::fromUtf8("closeButton"));
    closeButton->setMinimumSize(QSize(24, 24));
    closeButton->setMaximumSize(QSize(24, 24));
    closeButton->setFocusPolicy(Qt::NoFocus);

    auto* maxWinButton = new QToolButton(titleFrame);
    maxWinButton->setObjectName(QString::fromUtf8("maxWinButton"));
    maxWinButton->setMinimumSize(QSize(24, 24));
    maxWinButton->setMaximumSize(QSize(24, 24));
    maxWinButton->setFocusPolicy(Qt::NoFocus);

    auto* minWinButton = new QToolButton(titleFrame);
    minWinButton->setObjectName(QString::fromUtf8("minWinButton"));
    minWinButton->setMinimumSize(QSize(24, 24));
    minWinButton->setMaximumSize(QSize(24, 24));
    minWinButton->setFocusPolicy(Qt::NoFocus);
    minWinButton->setPopupMode(QToolButton::InstantPopup);

    auto* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* horizontalLayout = new QHBoxLayout(titleFrame);
    horizontalLayout->addItem(horizontalSpacer);
    horizontalLayout->addWidget(titleFrameLabel);
    horizontalLayout->addWidget(minWinButton);
    horizontalLayout->addWidget(maxWinButton);
    horizontalLayout->addWidget(closeButton);

    horizontalLayout->setSpacing(0);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->setContentsMargins(0, 0, 0, 0);

    default_layout->addWidget(titleFrame, 0, 1, 1, 3);

    /*auto* shadow = new QGraphicsDropShadowEffect(content_widget_);
    shadow->setOffset(0, 0);
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(20);
    content_widget_->setGraphicsEffect(shadow);*/

    default_layout->addWidget(content_widget_, 2, 1, 1, 2);

    closeButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         image: url(:/xamp/Resource/%1/close.png);
                                         background-color: transparent;
                                         }
                                         )").arg(ThemeManager::instance().themeColorPath()));

    minWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/minimize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(ThemeManager::instance().themeColorPath()));

    maxWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/maximize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(ThemeManager::instance().themeColorPath()));

    maxWinButton->setDisabled(true);
    minWinButton->setDisabled(true);

    (void)QObject::connect(closeButton, &QToolButton::pressed, [this]() {
        QWidget::close();
        emit closeFrame();
        });
}

