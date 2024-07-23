/********************************************************************************
** Form generated from reading UI file 'createplaylistdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CREATEPLAYLISTDIALOG_H
#define UI_CREATEPLAYLISTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_CreatePlaylistView
{
public:
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;
    QLineEdit *titleLineEdit;
    QSpacerItem *verticalSpacer_2;
    QLineEdit *descLineEdit;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout;
    QLabel *label_2;
    QComboBox *privateStatusComboBox;
    QSpacerItem *verticalSpacer_4;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_3;
    QDialogButtonBox *buttonBox;
    QHBoxLayout *horizontalLayout_2;

    void setupUi(QFrame *CreatePlaylistView)
    {
        if (CreatePlaylistView->objectName().isEmpty())
            CreatePlaylistView->setObjectName("CreatePlaylistView");
        CreatePlaylistView->resize(500, 300);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CreatePlaylistView->sizePolicy().hasHeightForWidth());
        CreatePlaylistView->setSizePolicy(sizePolicy);
        CreatePlaylistView->setMinimumSize(QSize(500, 300));
        CreatePlaylistView->setMaximumSize(QSize(16777215, 300));
        verticalLayout_2 = new QVBoxLayout(CreatePlaylistView);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout->addItem(verticalSpacer);

        titleLineEdit = new QLineEdit(CreatePlaylistView);
        titleLineEdit->setObjectName("titleLineEdit");

        verticalLayout->addWidget(titleLineEdit);

        verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout->addItem(verticalSpacer_2);

        descLineEdit = new QLineEdit(CreatePlaylistView);
        descLineEdit->setObjectName("descLineEdit");

        verticalLayout->addWidget(descLineEdit);

        verticalSpacer_3 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout->addItem(verticalSpacer_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_2 = new QLabel(CreatePlaylistView);
        label_2->setObjectName("label_2");

        horizontalLayout->addWidget(label_2);

        privateStatusComboBox = new QComboBox(CreatePlaylistView);
        privateStatusComboBox->addItem(QString());
        privateStatusComboBox->addItem(QString());
        privateStatusComboBox->addItem(QString());
        privateStatusComboBox->setObjectName("privateStatusComboBox");

        horizontalLayout->addWidget(privateStatusComboBox);

        horizontalLayout->setStretch(1, 1);

        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer_4);


        horizontalLayout_4->addLayout(verticalLayout);

        horizontalSpacer_2 = new QSpacerItem(100, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);


        verticalLayout_2->addLayout(horizontalLayout_4);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        buttonBox = new QDialogButtonBox(CreatePlaylistView);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout_3->addWidget(buttonBox);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");

        horizontalLayout_3->addLayout(horizontalLayout_2);


        verticalLayout_2->addLayout(horizontalLayout_3);

        verticalLayout_2->setStretch(0, 1);

        retranslateUi(CreatePlaylistView);

        QMetaObject::connectSlotsByName(CreatePlaylistView);
    } // setupUi

    void retranslateUi(QFrame *CreatePlaylistView)
    {
        CreatePlaylistView->setWindowTitle(QCoreApplication::translate("CreatePlaylistView", "Create Playlist", nullptr));
        titleLineEdit->setPlaceholderText(QCoreApplication::translate("CreatePlaylistView", "title", nullptr));
        descLineEdit->setPlaceholderText(QCoreApplication::translate("CreatePlaylistView", "description", nullptr));
        label_2->setText(QCoreApplication::translate("CreatePlaylistView", "Private Status:", nullptr));
        privateStatusComboBox->setItemText(0, QCoreApplication::translate("CreatePlaylistView", "PUBLIC", nullptr));
        privateStatusComboBox->setItemText(1, QCoreApplication::translate("CreatePlaylistView", "PRIVATE", nullptr));
        privateStatusComboBox->setItemText(2, QCoreApplication::translate("CreatePlaylistView", "UNLISTED", nullptr));

    } // retranslateUi

};

namespace Ui {
    class CreatePlaylistView: public Ui_CreatePlaylistView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CREATEPLAYLISTDIALOG_H
