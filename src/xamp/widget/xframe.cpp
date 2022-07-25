#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QToolButton>
#include <widget/xframe.h>

#include "str_utilts.h"
#include "thememanager.h"

XFrame::XFrame(QWidget* parent)
    : QFrame(parent) {
    setObjectName(Q_TEXT("XFrame"));
    setStyleSheet(Q_STR(R"(
			QFrame#XFrame {
				border-radius: 8px;
				border: 2px solid %1;
				background-color: %1;
            }
            )").arg(qTheme.backgroundColor()));
}

void XFrame::setTitle(const QString& title) const {
    title_frame_label->setText(title);
}

void XFrame::setContentWidget(QWidget* content) {
    content_ = content;

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(5, 5, 5, 5);
    setLayout(default_layout);

    auto* title_frame = new QFrame(this);
    title_frame->setObjectName(QString::fromUtf8("titleFrame"));
    title_frame->setMinimumSize(QSize(0, 24));
    title_frame->setFrameShape(QFrame::NoFrame);
    title_frame->setFrameShadow(QFrame::Plain);
    title_frame->setStyleSheet(Q_TEXT("QFrame#titleFrame { border: none; }"));

    auto f = font();
    f.setBold(true);
    f.setPointSize(12);
    title_frame_label = new QLabel(title_frame);
    title_frame_label->setObjectName(QString::fromUtf8("titleFrameLabel"));
    QSizePolicy size_policy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    size_policy3.setHorizontalStretch(1);
    size_policy3.setVerticalStretch(0);
    size_policy3.setHeightForWidth(title_frame_label->sizePolicy().hasHeightForWidth());
    title_frame_label->setFont(f);
    title_frame_label->setSizePolicy(size_policy3);
    title_frame_label->setAlignment(Qt::AlignCenter);
    title_frame_label->setStyleSheet(Q_STR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

    auto* close_button = new QToolButton(title_frame);
    close_button->setObjectName(QString::fromUtf8("closeButton"));
    close_button->setMinimumSize(QSize(24, 24));
    close_button->setMaximumSize(QSize(24, 24));
    close_button->setFocusPolicy(Qt::NoFocus);

    auto* max_win_button = new QToolButton(title_frame);
    max_win_button->setObjectName(QString::fromUtf8("maxWinButton"));
    max_win_button->setMinimumSize(QSize(24, 24));
    max_win_button->setMaximumSize(QSize(24, 24));
    max_win_button->setFocusPolicy(Qt::NoFocus);

    auto* min_win_button = new QToolButton(title_frame);
    min_win_button->setObjectName(QString::fromUtf8("minWinButton"));
    min_win_button->setMinimumSize(QSize(24, 24));
    min_win_button->setMaximumSize(QSize(24, 24));
    min_win_button->setFocusPolicy(Qt::NoFocus);
    min_win_button->setPopupMode(QToolButton::InstantPopup);

    auto* horizontal_spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* horizontal_layout = new QHBoxLayout(title_frame);
    horizontal_layout->addItem(horizontal_spacer);
    horizontal_layout->addWidget(title_frame_label);
    horizontal_layout->addWidget(min_win_button);
    horizontal_layout->addWidget(max_win_button);
    horizontal_layout->addWidget(close_button);

    horizontal_layout->setSpacing(0);
    horizontal_layout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontal_layout->setContentsMargins(0, 0, 0, 0);

    default_layout->addWidget(title_frame, 1);

    default_layout->addWidget(content_, 0);

    close_button->setStyleSheet(Q_STR(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         image: url(:/xamp/Resource/%1/close_normal.png);
                                         background-color: transparent;
                                         }

										 QToolButton#closeButton:hover {	
										 image: url(:/xamp/Resource/%1/close_hover.png);									 
                                         }
                                         )").arg(qTheme.themeColorPath()));

    min_win_button->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/minimize_normal.png);
                                          background-color: transparent;
                                          }
										  QToolButton#minWinButton:hover {	
										  image: url(:/xamp/Resource/%1/minimize_hover.png);									 
                                          }
                                          )").arg(qTheme.themeColorPath()));

    max_win_button->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/maximize_normal.png);
                                          background-color: transparent;
                                          }
										  QToolButton#maxWinButton:hover {	
										  image: url(:/xamp/Resource/%1/maximize_hover.png);									 
                                          }
                                          )").arg(qTheme.themeColorPath()));

    max_win_button->setDisabled(true);
    min_win_button->setDisabled(true);

    max_win_button->hide();
    min_win_button->hide();

    (void)QObject::connect(close_button, &QToolButton::pressed, [this]() {
        QWidget::close();
        emit closeFrame();
        });
}

