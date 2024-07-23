/********************************************************************************
** Form generated from reading UI file 'volumecontroldialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VOLUMECONTROLDIALOG_H
#define UI_VOLUMECONTROLDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include "widget/seekslider.h"

QT_BEGIN_NAMESPACE

class Ui_VolumeControlDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *volumeLabel;
    QHBoxLayout *horizontalLayout;
    SeekSlider *volumeSlider;

    void setupUi(QDialog *VolumeControlDialog)
    {
        if (VolumeControlDialog->objectName().isEmpty())
            VolumeControlDialog->setObjectName("VolumeControlDialog");
        VolumeControlDialog->resize(56, 175);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(VolumeControlDialog->sizePolicy().hasHeightForWidth());
        VolumeControlDialog->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(VolumeControlDialog);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 6);
        volumeLabel = new QLabel(VolumeControlDialog);
        volumeLabel->setObjectName("volumeLabel");
        volumeLabel->setMinimumSize(QSize(0, 24));
        volumeLabel->setMaximumSize(QSize(16777215, 24));
        volumeLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(volumeLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        volumeSlider = new SeekSlider(VolumeControlDialog);
        volumeSlider->setObjectName("volumeSlider");
        volumeSlider->setOrientation(Qt::Vertical);

        horizontalLayout->addWidget(volumeSlider);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(VolumeControlDialog);

        QMetaObject::connectSlotsByName(VolumeControlDialog);
    } // setupUi

    void retranslateUi(QDialog *VolumeControlDialog)
    {
        VolumeControlDialog->setWindowTitle(QCoreApplication::translate("VolumeControlDialog", "Dialog", nullptr));
        volumeLabel->setText(QCoreApplication::translate("VolumeControlDialog", "0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class VolumeControlDialog: public Ui_VolumeControlDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VOLUMECONTROLDIALOG_H
