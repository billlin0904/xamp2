/********************************************************************************
** Form generated from reading UI file 'accountauthorizationpage.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ACCOUNTAUTHORIZATIONPAGE_H
#define UI_ACCOUNTAUTHORIZATIONPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AccountAuthorizationPage
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *accountLabel;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_4;
    QLabel *expiresAtLabel;
    QLabel *expiresAtTimeLabel;
    QHBoxLayout *horizontalLayout_3;
    QLabel *expiresInLabel;
    QLabel *expiresInTimeLabel;
    QSpacerItem *verticalSpacer;

    void setupUi(QFrame *AccountAuthorizationPage)
    {
        if (AccountAuthorizationPage->objectName().isEmpty())
            AccountAuthorizationPage->setObjectName("AccountAuthorizationPage");
        AccountAuthorizationPage->resize(500, 400);
        AccountAuthorizationPage->setMinimumSize(QSize(500, 400));
        verticalLayout = new QVBoxLayout(AccountAuthorizationPage);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        accountLabel = new QPushButton(AccountAuthorizationPage);
        accountLabel->setObjectName("accountLabel");
        accountLabel->setMinimumSize(QSize(64, 64));
        accountLabel->setMaximumSize(QSize(64, 64));
        accountLabel->setIconSize(QSize(64, 64));

        horizontalLayout->addWidget(accountLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        expiresAtLabel = new QLabel(AccountAuthorizationPage);
        expiresAtLabel->setObjectName("expiresAtLabel");
        expiresAtLabel->setMaximumSize(QSize(70, 15));
        QFont font;
        font.setBold(true);
        expiresAtLabel->setFont(font);

        horizontalLayout_4->addWidget(expiresAtLabel);

        expiresAtTimeLabel = new QLabel(AccountAuthorizationPage);
        expiresAtTimeLabel->setObjectName("expiresAtTimeLabel");

        horizontalLayout_4->addWidget(expiresAtTimeLabel);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        expiresInLabel = new QLabel(AccountAuthorizationPage);
        expiresInLabel->setObjectName("expiresInLabel");
        expiresInLabel->setMaximumSize(QSize(70, 15));
        expiresInLabel->setFont(font);

        horizontalLayout_3->addWidget(expiresInLabel);

        expiresInTimeLabel = new QLabel(AccountAuthorizationPage);
        expiresInTimeLabel->setObjectName("expiresInTimeLabel");

        horizontalLayout_3->addWidget(expiresInTimeLabel);


        verticalLayout->addLayout(horizontalLayout_3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(AccountAuthorizationPage);

        QMetaObject::connectSlotsByName(AccountAuthorizationPage);
    } // setupUi

    void retranslateUi(QFrame *AccountAuthorizationPage)
    {
        AccountAuthorizationPage->setWindowTitle(QCoreApplication::translate("AccountAuthorizationPage", "Account Authorization", nullptr));
        accountLabel->setText(QString());
        expiresAtLabel->setText(QCoreApplication::translate("AccountAuthorizationPage", "Expires at:", nullptr));
        expiresAtTimeLabel->setText(QCoreApplication::translate("AccountAuthorizationPage", "Unauthorized", nullptr));
        expiresInLabel->setText(QCoreApplication::translate("AccountAuthorizationPage", "Expires in:", nullptr));
        expiresInTimeLabel->setText(QCoreApplication::translate("AccountAuthorizationPage", "Unauthorized", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AccountAuthorizationPage: public Ui_AccountAuthorizationPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ACCOUNTAUTHORIZATIONPAGE_H
