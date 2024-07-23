/********************************************************************************
** Form generated from reading UI file 'equalizerdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EQUALIZERDIALOG_H
#define UI_EQUALIZERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "widget/doubleslider.h"

QT_BEGIN_NAMESPACE

class Ui_EqualizerView
{
public:
    QVBoxLayout *verticalLayout_13;
    QHBoxLayout *horizontalLayout;
    QLabel *presetLabel;
    QComboBox *eqPresetComboBox;
    QCheckBox *enableEqCheckBox;
    QPushButton *resetButton;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_3;
    QLabel *band3DbLabel;
    DoubleSlider *band3Slider;
    QLabel *band3FeqLabel;
    QVBoxLayout *verticalLayout_12;
    QLabel *preampDbLabel;
    DoubleSlider *preampSlider;
    QLabel *preampLabel;
    QVBoxLayout *verticalLayout_9;
    QLabel *band9DbLabel;
    DoubleSlider *band9Slider;
    QLabel *band9FeqLabel;
    QVBoxLayout *verticalLayout_8;
    QLabel *band8DbLabel;
    DoubleSlider *band8Slider;
    QLabel *band8FeqLabel;
    QVBoxLayout *verticalLayout_7;
    QLabel *band7DbLabel;
    DoubleSlider *band7Slider;
    QLabel *band7FeqLabel;
    QSpacerItem *horizontalSpacer_2;
    QVBoxLayout *verticalLayout_10;
    QLabel *band10DbLabel;
    DoubleSlider *band10Slider;
    QLabel *band10FeqLabel;
    QVBoxLayout *verticalLayout_2;
    QLabel *band2DbLabel;
    DoubleSlider *band2Slider;
    QLabel *band2FeqLabel;
    QVBoxLayout *verticalLayout_6;
    QLabel *band6DbLabel;
    DoubleSlider *band6Slider;
    QLabel *band6FeqLabel;
    QVBoxLayout *verticalLayout;
    QLabel *band1DbLabel;
    DoubleSlider *band1Slider;
    QLabel *band1FeqLabel;
    QVBoxLayout *verticalLayout_4;
    QLabel *band4DbLabel;
    DoubleSlider *band4Slider;
    QLabel *band4FeqLabel;
    QVBoxLayout *verticalLayout_5;
    QLabel *band5DbLabel;
    DoubleSlider *band5Slider;
    QLabel *band5FeqLabel;
    QSpacerItem *horizontalSpacer_3;

    void setupUi(QFrame *EqualizerView)
    {
        if (EqualizerView->objectName().isEmpty())
            EqualizerView->setObjectName("EqualizerView");
        EqualizerView->resize(778, 300);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(EqualizerView->sizePolicy().hasHeightForWidth());
        EqualizerView->setSizePolicy(sizePolicy);
        EqualizerView->setMinimumSize(QSize(0, 300));
        EqualizerView->setMaximumSize(QSize(16777215, 300));
        verticalLayout_13 = new QVBoxLayout(EqualizerView);
        verticalLayout_13->setObjectName("verticalLayout_13");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        presetLabel = new QLabel(EqualizerView);
        presetLabel->setObjectName("presetLabel");
        QFont font;
        font.setBold(true);
        presetLabel->setFont(font);

        horizontalLayout->addWidget(presetLabel);

        eqPresetComboBox = new QComboBox(EqualizerView);
        eqPresetComboBox->setObjectName("eqPresetComboBox");

        horizontalLayout->addWidget(eqPresetComboBox);

        enableEqCheckBox = new QCheckBox(EqualizerView);
        enableEqCheckBox->setObjectName("enableEqCheckBox");

        horizontalLayout->addWidget(enableEqCheckBox);

        resetButton = new QPushButton(EqualizerView);
        resetButton->setObjectName("resetButton");

        horizontalLayout->addWidget(resetButton);

        horizontalLayout->setStretch(1, 1);

        verticalLayout_13->addLayout(horizontalLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setContentsMargins(1, -1, -1, -1);
        band3DbLabel = new QLabel(EqualizerView);
        band3DbLabel->setObjectName("band3DbLabel");
        band3DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_3->addWidget(band3DbLabel);

        band3Slider = new DoubleSlider(EqualizerView);
        band3Slider->setObjectName("band3Slider");
        band3Slider->setMinimum(-150);
        band3Slider->setMaximum(150);
        band3Slider->setOrientation(Qt::Vertical);
        band3Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_3->addWidget(band3Slider);

        band3FeqLabel = new QLabel(EqualizerView);
        band3FeqLabel->setObjectName("band3FeqLabel");
        band3FeqLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(band3FeqLabel);


        gridLayout->addLayout(verticalLayout_3, 0, 4, 1, 1);

        verticalLayout_12 = new QVBoxLayout();
        verticalLayout_12->setSpacing(0);
        verticalLayout_12->setObjectName("verticalLayout_12");
        verticalLayout_12->setContentsMargins(1, -1, -1, -1);
        preampDbLabel = new QLabel(EqualizerView);
        preampDbLabel->setObjectName("preampDbLabel");
        preampDbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_12->addWidget(preampDbLabel);

        preampSlider = new DoubleSlider(EqualizerView);
        preampSlider->setObjectName("preampSlider");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(preampSlider->sizePolicy().hasHeightForWidth());
        preampSlider->setSizePolicy(sizePolicy1);
        preampSlider->setMinimum(-400);
        preampSlider->setMaximum(400);
        preampSlider->setOrientation(Qt::Vertical);
        preampSlider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_12->addWidget(preampSlider);

        preampLabel = new QLabel(EqualizerView);
        preampLabel->setObjectName("preampLabel");
        preampLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_12->addWidget(preampLabel);


        gridLayout->addLayout(verticalLayout_12, 0, 0, 1, 1);

        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setSpacing(0);
        verticalLayout_9->setObjectName("verticalLayout_9");
        verticalLayout_9->setContentsMargins(1, -1, -1, -1);
        band9DbLabel = new QLabel(EqualizerView);
        band9DbLabel->setObjectName("band9DbLabel");
        band9DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_9->addWidget(band9DbLabel);

        band9Slider = new DoubleSlider(EqualizerView);
        band9Slider->setObjectName("band9Slider");
        band9Slider->setMinimum(-150);
        band9Slider->setMaximum(150);
        band9Slider->setOrientation(Qt::Vertical);
        band9Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_9->addWidget(band9Slider);

        band9FeqLabel = new QLabel(EqualizerView);
        band9FeqLabel->setObjectName("band9FeqLabel");
        band9FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_9->addWidget(band9FeqLabel);


        gridLayout->addLayout(verticalLayout_9, 0, 10, 1, 1);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setObjectName("verticalLayout_8");
        verticalLayout_8->setContentsMargins(1, -1, -1, -1);
        band8DbLabel = new QLabel(EqualizerView);
        band8DbLabel->setObjectName("band8DbLabel");
        band8DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_8->addWidget(band8DbLabel);

        band8Slider = new DoubleSlider(EqualizerView);
        band8Slider->setObjectName("band8Slider");
        band8Slider->setMinimum(-150);
        band8Slider->setMaximum(150);
        band8Slider->setOrientation(Qt::Vertical);
        band8Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_8->addWidget(band8Slider);

        band8FeqLabel = new QLabel(EqualizerView);
        band8FeqLabel->setObjectName("band8FeqLabel");
        band8FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_8->addWidget(band8FeqLabel);


        gridLayout->addLayout(verticalLayout_8, 0, 9, 1, 1);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName("verticalLayout_7");
        verticalLayout_7->setContentsMargins(1, -1, -1, -1);
        band7DbLabel = new QLabel(EqualizerView);
        band7DbLabel->setObjectName("band7DbLabel");
        band7DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_7->addWidget(band7DbLabel);

        band7Slider = new DoubleSlider(EqualizerView);
        band7Slider->setObjectName("band7Slider");
        band7Slider->setMinimum(-150);
        band7Slider->setMaximum(150);
        band7Slider->setOrientation(Qt::Vertical);
        band7Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_7->addWidget(band7Slider);

        band7FeqLabel = new QLabel(EqualizerView);
        band7FeqLabel->setObjectName("band7FeqLabel");
        band7FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_7->addWidget(band7FeqLabel);


        gridLayout->addLayout(verticalLayout_7, 0, 8, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 1, 1, 1);

        verticalLayout_10 = new QVBoxLayout();
        verticalLayout_10->setSpacing(0);
        verticalLayout_10->setObjectName("verticalLayout_10");
        verticalLayout_10->setContentsMargins(1, -1, 0, -1);
        band10DbLabel = new QLabel(EqualizerView);
        band10DbLabel->setObjectName("band10DbLabel");
        band10DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_10->addWidget(band10DbLabel);

        band10Slider = new DoubleSlider(EqualizerView);
        band10Slider->setObjectName("band10Slider");
        band10Slider->setMinimum(-150);
        band10Slider->setMaximum(150);
        band10Slider->setOrientation(Qt::Vertical);
        band10Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_10->addWidget(band10Slider);

        band10FeqLabel = new QLabel(EqualizerView);
        band10FeqLabel->setObjectName("band10FeqLabel");
        band10FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_10->addWidget(band10FeqLabel);


        gridLayout->addLayout(verticalLayout_10, 0, 11, 1, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(1, -1, -1, -1);
        band2DbLabel = new QLabel(EqualizerView);
        band2DbLabel->setObjectName("band2DbLabel");
        band2DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_2->addWidget(band2DbLabel);

        band2Slider = new DoubleSlider(EqualizerView);
        band2Slider->setObjectName("band2Slider");
        band2Slider->setMinimum(-150);
        band2Slider->setMaximum(150);
        band2Slider->setOrientation(Qt::Vertical);
        band2Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_2->addWidget(band2Slider);

        band2FeqLabel = new QLabel(EqualizerView);
        band2FeqLabel->setObjectName("band2FeqLabel");
        band2FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_2->addWidget(band2FeqLabel);


        gridLayout->addLayout(verticalLayout_2, 0, 3, 1, 1);

        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName("verticalLayout_6");
        verticalLayout_6->setContentsMargins(1, -1, -1, -1);
        band6DbLabel = new QLabel(EqualizerView);
        band6DbLabel->setObjectName("band6DbLabel");
        band6DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_6->addWidget(band6DbLabel);

        band6Slider = new DoubleSlider(EqualizerView);
        band6Slider->setObjectName("band6Slider");
        band6Slider->setMinimum(-150);
        band6Slider->setMaximum(150);
        band6Slider->setOrientation(Qt::Vertical);
        band6Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_6->addWidget(band6Slider);

        band6FeqLabel = new QLabel(EqualizerView);
        band6FeqLabel->setObjectName("band6FeqLabel");
        band6FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_6->addWidget(band6FeqLabel);


        gridLayout->addLayout(verticalLayout_6, 0, 7, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(1, -1, -1, -1);
        band1DbLabel = new QLabel(EqualizerView);
        band1DbLabel->setObjectName("band1DbLabel");
        band1DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout->addWidget(band1DbLabel);

        band1Slider = new DoubleSlider(EqualizerView);
        band1Slider->setObjectName("band1Slider");
        band1Slider->setMinimum(-150);
        band1Slider->setMaximum(150);
        band1Slider->setOrientation(Qt::Vertical);
        band1Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout->addWidget(band1Slider);

        band1FeqLabel = new QLabel(EqualizerView);
        band1FeqLabel->setObjectName("band1FeqLabel");
        band1FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout->addWidget(band1FeqLabel);


        gridLayout->addLayout(verticalLayout, 0, 2, 1, 1);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(1, -1, -1, -1);
        band4DbLabel = new QLabel(EqualizerView);
        band4DbLabel->setObjectName("band4DbLabel");
        band4DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_4->addWidget(band4DbLabel);

        band4Slider = new DoubleSlider(EqualizerView);
        band4Slider->setObjectName("band4Slider");
        band4Slider->setMinimum(-150);
        band4Slider->setMaximum(150);
        band4Slider->setOrientation(Qt::Vertical);
        band4Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_4->addWidget(band4Slider);

        band4FeqLabel = new QLabel(EqualizerView);
        band4FeqLabel->setObjectName("band4FeqLabel");
        band4FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_4->addWidget(band4FeqLabel);


        gridLayout->addLayout(verticalLayout_4, 0, 5, 1, 1);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(0);
        verticalLayout_5->setObjectName("verticalLayout_5");
        verticalLayout_5->setContentsMargins(1, -1, -1, -1);
        band5DbLabel = new QLabel(EqualizerView);
        band5DbLabel->setObjectName("band5DbLabel");
        band5DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_5->addWidget(band5DbLabel);

        band5Slider = new DoubleSlider(EqualizerView);
        band5Slider->setObjectName("band5Slider");
        band5Slider->setMinimum(-150);
        band5Slider->setMaximum(150);
        band5Slider->setOrientation(Qt::Vertical);
        band5Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_5->addWidget(band5Slider);

        band5FeqLabel = new QLabel(EqualizerView);
        band5FeqLabel->setObjectName("band5FeqLabel");
        band5FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_5->addWidget(band5FeqLabel);


        gridLayout->addLayout(verticalLayout_5, 0, 6, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 0, 12, 1, 1);


        horizontalLayout_3->addLayout(gridLayout);

        horizontalLayout_3->setStretch(1, 1);

        verticalLayout_13->addLayout(horizontalLayout_3);

        verticalLayout_13->setStretch(1, 1);

        retranslateUi(EqualizerView);

        QMetaObject::connectSlotsByName(EqualizerView);
    } // setupUi

    void retranslateUi(QFrame *EqualizerView)
    {
        EqualizerView->setWindowTitle(QCoreApplication::translate("EqualizerView", "Equalizer", nullptr));
        presetLabel->setText(QCoreApplication::translate("EqualizerView", "Preset:", nullptr));
        enableEqCheckBox->setText(QCoreApplication::translate("EqualizerView", "Eanble", nullptr));
        resetButton->setText(QCoreApplication::translate("EqualizerView", "Reset", nullptr));
        band3DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band3FeqLabel->setText(QCoreApplication::translate("EqualizerView", "125Hz", nullptr));
        preampDbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        preampLabel->setText(QCoreApplication::translate("EqualizerView", "Preamp", nullptr));
        band9DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band9FeqLabel->setText(QCoreApplication::translate("EqualizerView", "4KHz", nullptr));
        band8DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band8FeqLabel->setText(QCoreApplication::translate("EqualizerView", "2KHz", nullptr));
        band7DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band7FeqLabel->setText(QCoreApplication::translate("EqualizerView", "1KHz", nullptr));
        band10DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band10FeqLabel->setText(QCoreApplication::translate("EqualizerView", "8KHz", nullptr));
        band2DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band2FeqLabel->setText(QCoreApplication::translate("EqualizerView", "62Hz", nullptr));
        band6DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band6FeqLabel->setText(QCoreApplication::translate("EqualizerView", "500Hz", nullptr));
        band1DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band1FeqLabel->setText(QCoreApplication::translate("EqualizerView", "31.1Hz", nullptr));
        band4DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band4FeqLabel->setText(QCoreApplication::translate("EqualizerView", "125Hz", nullptr));
        band5DbLabel->setText(QCoreApplication::translate("EqualizerView", "0db", nullptr));
        band5FeqLabel->setText(QCoreApplication::translate("EqualizerView", "250Hz", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EqualizerView: public Ui_EqualizerView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EQUALIZERDIALOG_H
