/********************************************************************************
** Form generated from reading UI file 'cdpage.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CDPAGE_H
#define UI_CDPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include "widget/playlistpage.h"

QT_BEGIN_NAMESPACE

class Ui_CDPage
{
public:
    QVBoxLayout *verticalLayout_3;
    PlaylistPage *playlistPage;
    QFrame *tipFrame;
    QVBoxLayout *verticalLayout_2;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QToolButton *pcButton;
    QToolButton *arrowButton;
    QToolButton *cdButton;
    QSpacerItem *horizontalSpacer_2;
    QLabel *hintLabel;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QFrame *CDPage)
    {
        if (CDPage->objectName().isEmpty())
            CDPage->setObjectName("CDPage");
        CDPage->resize(779, 589);
        verticalLayout_3 = new QVBoxLayout(CDPage);
        verticalLayout_3->setObjectName("verticalLayout_3");
        playlistPage = new PlaylistPage(CDPage);
        playlistPage->setObjectName("playlistPage");
        playlistPage->setFrameShape(QFrame::StyledPanel);
        playlistPage->setFrameShadow(QFrame::Raised);

        verticalLayout_3->addWidget(playlistPage);

        tipFrame = new QFrame(CDPage);
        tipFrame->setObjectName("tipFrame");
        tipFrame->setFrameShape(QFrame::StyledPanel);
        tipFrame->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(tipFrame);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalSpacer = new QSpacerItem(20, 242, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        pcButton = new QToolButton(tipFrame);
        pcButton->setObjectName("pcButton");
        pcButton->setMinimumSize(QSize(48, 48));
        pcButton->setMaximumSize(QSize(48, 48));

        horizontalLayout->addWidget(pcButton);

        arrowButton = new QToolButton(tipFrame);
        arrowButton->setObjectName("arrowButton");
        arrowButton->setMinimumSize(QSize(48, 48));
        arrowButton->setMaximumSize(QSize(48, 48));

        horizontalLayout->addWidget(arrowButton);

        cdButton = new QToolButton(tipFrame);
        cdButton->setObjectName("cdButton");
        cdButton->setMinimumSize(QSize(48, 48));
        cdButton->setMaximumSize(QSize(48, 48));

        horizontalLayout->addWidget(cdButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        hintLabel = new QLabel(tipFrame);
        hintLabel->setObjectName("hintLabel");
        hintLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(hintLabel);


        verticalLayout_2->addLayout(verticalLayout);

        verticalSpacer_2 = new QSpacerItem(20, 240, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);


        verticalLayout_3->addWidget(tipFrame);


        retranslateUi(CDPage);

        QMetaObject::connectSlotsByName(CDPage);
    } // setupUi

    void retranslateUi(QFrame *CDPage)
    {
        CDPage->setWindowTitle(QCoreApplication::translate("CDPage", "Frame", nullptr));
        pcButton->setText(QString());
        arrowButton->setText(QString());
        cdButton->setText(QString());
        hintLabel->setText(QCoreApplication::translate("CDPage", "Please Insert CD", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CDPage: public Ui_CDPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CDPAGE_H
