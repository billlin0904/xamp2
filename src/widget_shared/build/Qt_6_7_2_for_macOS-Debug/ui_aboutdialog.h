/********************************************************************************
** Form generated from reading UI file 'aboutdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <widget/processindicator.h>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer_3;
    QLabel *lblLogo;
    QSpacerItem *verticalSpacer_4;
    QLabel *lblProjectTitle;
    QLabel *lblDescription;
    QLabel *lblCopying;
    QWidget *wdtContent;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QLabel *lbIGithubIcon;
    QLabel *lblDomain;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_4;
    ProcessIndicator *waitForUpdateProcessIndicator;
    QLabel *lblAppBuild;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *restartAppButton;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *horizontalSpacer_6;
    QSpacerItem *verticalSpacer_2;
    QTextBrowser *txtBws;
    QHBoxLayout *horizontalLayout;
    QPushButton *btnCredits;
    QPushButton *btnLicense;
    QSpacerItem *horizontalSpacer;

    void setupUi(QFrame *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName("AboutDialog");
        AboutDialog->resize(502, 605);
        AboutDialog->setProperty("sizeGripEnabled", QVariant(false));
        verticalLayout = new QVBoxLayout(AboutDialog);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(10, 10, 10, 10);
        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout->addItem(verticalSpacer_3);

        lblLogo = new QLabel(AboutDialog);
        lblLogo->setObjectName("lblLogo");

        verticalLayout->addWidget(lblLogo, 0, Qt::AlignHCenter);

        verticalSpacer_4 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout->addItem(verticalSpacer_4);

        lblProjectTitle = new QLabel(AboutDialog);
        lblProjectTitle->setObjectName("lblProjectTitle");
        lblProjectTitle->setMinimumSize(QSize(0, 50));
        lblProjectTitle->setMaximumSize(QSize(16777215, 50));

        verticalLayout->addWidget(lblProjectTitle, 0, Qt::AlignHCenter);

        lblDescription = new QLabel(AboutDialog);
        lblDescription->setObjectName("lblDescription");
        lblDescription->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(lblDescription);

        lblCopying = new QLabel(AboutDialog);
        lblCopying->setObjectName("lblCopying");
        lblCopying->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(lblCopying);

        wdtContent = new QWidget(AboutDialog);
        wdtContent->setObjectName("wdtContent");
        verticalLayout_2 = new QVBoxLayout(wdtContent);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(5);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        lbIGithubIcon = new QLabel(wdtContent);
        lbIGithubIcon->setObjectName("lbIGithubIcon");

        horizontalLayout_2->addWidget(lbIGithubIcon);

        lblDomain = new QLabel(wdtContent);
        lblDomain->setObjectName("lblDomain");
        lblDomain->setTextFormat(Qt::RichText);
        lblDomain->setOpenExternalLinks(true);
        lblDomain->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

        horizontalLayout_2->addWidget(lblDomain);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);


        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_4);

        waitForUpdateProcessIndicator = new ProcessIndicator(wdtContent);
        waitForUpdateProcessIndicator->setObjectName("waitForUpdateProcessIndicator");
        waitForUpdateProcessIndicator->setMinimumSize(QSize(24, 24));
        waitForUpdateProcessIndicator->setMaximumSize(QSize(24, 24));

        horizontalLayout_3->addWidget(waitForUpdateProcessIndicator);

        lblAppBuild = new QLabel(wdtContent);
        lblAppBuild->setObjectName("lblAppBuild");
        lblAppBuild->setMinimumSize(QSize(200, 0));
        lblAppBuild->setMaximumSize(QSize(200, 16777215));
        lblAppBuild->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        horizontalLayout_3->addWidget(lblAppBuild);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_5);

        restartAppButton = new QPushButton(wdtContent);
        restartAppButton->setObjectName("restartAppButton");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(restartAppButton->sizePolicy().hasHeightForWidth());
        restartAppButton->setSizePolicy(sizePolicy);
        restartAppButton->setMinimumSize(QSize(0, 23));
        restartAppButton->setMaximumSize(QSize(16777215, 23));
        restartAppButton->setCheckable(true);
        restartAppButton->setChecked(false);
        restartAppButton->setAutoDefault(true);
        restartAppButton->setFlat(false);

        horizontalLayout_3->addWidget(restartAppButton);

        horizontalSpacer_7 = new QSpacerItem(60, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_7);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_6);


        verticalLayout_2->addLayout(horizontalLayout_3);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_2->addItem(verticalSpacer_2);


        verticalLayout->addWidget(wdtContent);

        txtBws = new QTextBrowser(AboutDialog);
        txtBws->setObjectName("txtBws");
        txtBws->setTabChangesFocus(true);
        txtBws->setOpenExternalLinks(true);

        verticalLayout->addWidget(txtBws);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(5);
        horizontalLayout->setObjectName("horizontalLayout");
        btnCredits = new QPushButton(AboutDialog);
        btnCredits->setObjectName("btnCredits");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(btnCredits->sizePolicy().hasHeightForWidth());
        btnCredits->setSizePolicy(sizePolicy1);
        btnCredits->setCheckable(true);
        btnCredits->setChecked(false);
        btnCredits->setAutoDefault(true);
        btnCredits->setFlat(false);

        horizontalLayout->addWidget(btnCredits);

        btnLicense = new QPushButton(AboutDialog);
        btnLicense->setObjectName("btnLicense");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(btnLicense->sizePolicy().hasHeightForWidth());
        btnLicense->setSizePolicy(sizePolicy2);
        btnLicense->setCheckable(true);

        horizontalLayout->addWidget(btnLicense);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(AboutDialog);

        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QFrame *AboutDialog)
    {
        AboutDialog->setWindowTitle(QCoreApplication::translate("AboutDialog", "About", nullptr));
        lblLogo->setText(QCoreApplication::translate("AboutDialog", "logo", nullptr));
        lblProjectTitle->setText(QCoreApplication::translate("AboutDialog", "project_title", nullptr));
        lblDescription->setText(QCoreApplication::translate("AboutDialog", "description", nullptr));
        lblCopying->setText(QCoreApplication::translate("AboutDialog", "copying", nullptr));
        lbIGithubIcon->setText(QCoreApplication::translate("AboutDialog", "github_icon", nullptr));
        lblDomain->setText(QCoreApplication::translate("AboutDialog", "domain", nullptr));
        lblAppBuild->setText(QCoreApplication::translate("AboutDialog", "build", nullptr));
        restartAppButton->setText(QCoreApplication::translate("AboutDialog", "Restart", nullptr));
        btnCredits->setText(QCoreApplication::translate("AboutDialog", "Credits", nullptr));
        btnLicense->setText(QCoreApplication::translate("AboutDialog", "License", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
