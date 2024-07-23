/********************************************************************************
** Form generated from reading UI file 'supereqdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SUPEREQDIALOG_H
#define UI_SUPEREQDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include "widget/doubleslider.h"

QT_BEGIN_NAMESPACE

class Ui_SuperEqView
{
public:
    QVBoxLayout *verticalLayout_13;
    QHBoxLayout *horizontalLayout;
    QLabel *label_23;
    QComboBox *eqPresetComboBox;
    QCheckBox *enableEqCheckBox;
    QPushButton *resetButton;
    QPushButton *saveButton;
    QPushButton *deleteButton;
    QHBoxLayout *horizontalLayout_3;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_17;
    QLabel *band10DbLabel;
    DoubleSlider *band10Slider;
    QLabel *band10FeqLabel;
    QVBoxLayout *verticalLayout_4;
    QLabel *band7DbLabel;
    DoubleSlider *band7Slider;
    QLabel *band7FeqLabel;
    QVBoxLayout *verticalLayout_14;
    QLabel *band6DbLabel;
    DoubleSlider *band6Slider;
    QLabel *band6FeqLabel;
    QVBoxLayout *verticalLayout_3;
    QLabel *band5DbLabel;
    DoubleSlider *band5Slider;
    QLabel *band5FeqLabel;
    QVBoxLayout *verticalLayout_18;
    QLabel *band12DbLabel;
    DoubleSlider *band12Slider;
    QLabel *band12FeqLabel;
    QVBoxLayout *verticalLayout_7;
    QLabel *band13DbLabel;
    DoubleSlider *band13Slider;
    QLabel *band13FeqLabel;
    QVBoxLayout *verticalLayout_2;
    QLabel *band3DbLabel;
    DoubleSlider *band3Slider;
    QLabel *band3FeqLabel;
    QVBoxLayout *verticalLayout;
    QLabel *band1DbLabel;
    DoubleSlider *band1Slider;
    QLabel *band1FeqLabel;
    QVBoxLayout *verticalLayout_12;
    QLabel *band4DbLabel;
    DoubleSlider *band4Slider;
    QLabel *band4FeqLabel;
    QVBoxLayout *verticalLayout_9;
    QLabel *band17DbLabel;
    DoubleSlider *band17Slider;
    QLabel *band17FeqLabel;
    QVBoxLayout *verticalLayout_16;
    QLabel *band8DbLabel;
    DoubleSlider *band8Slider;
    QLabel *band8FeqLabel;
    QVBoxLayout *verticalLayout_20;
    QLabel *band16DbLabel;
    DoubleSlider *band16Slider;
    QLabel *band16FeqLabel;
    QVBoxLayout *verticalLayout_8;
    QLabel *band15DbLabel;
    DoubleSlider *band15Slider;
    QLabel *band15FeqLabel;
    QVBoxLayout *verticalLayout_10;
    QLabel *band18DbLabel;
    DoubleSlider *band18Slider;
    QLabel *band18FeqLabel;
    QVBoxLayout *verticalLayout_5;
    QLabel *band9DbLabel;
    DoubleSlider *band9Slider;
    QLabel *band9FeqLabel;
    QVBoxLayout *verticalLayout_6;
    QLabel *band11DbLabel;
    DoubleSlider *band11Slider;
    QLabel *band11FeqLabel;
    QVBoxLayout *verticalLayout_11;
    QLabel *band2DbLabel;
    DoubleSlider *band2Slider;
    QLabel *band2FeqLabel;
    QVBoxLayout *verticalLayout_19;
    QLabel *band14DbLabel;
    DoubleSlider *band14Slider;
    QLabel *band14FeqLabel;

    void setupUi(QFrame *SuperEqView)
    {
        if (SuperEqView->objectName().isEmpty())
            SuperEqView->setObjectName("SuperEqView");
        SuperEqView->resize(793, 300);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(SuperEqView->sizePolicy().hasHeightForWidth());
        SuperEqView->setSizePolicy(sizePolicy);
        SuperEqView->setMinimumSize(QSize(0, 300));
        SuperEqView->setMaximumSize(QSize(16777215, 300));
        verticalLayout_13 = new QVBoxLayout(SuperEqView);
        verticalLayout_13->setObjectName("verticalLayout_13");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_23 = new QLabel(SuperEqView);
        label_23->setObjectName("label_23");
        QFont font;
        font.setBold(true);
        label_23->setFont(font);

        horizontalLayout->addWidget(label_23);

        eqPresetComboBox = new QComboBox(SuperEqView);
        eqPresetComboBox->setObjectName("eqPresetComboBox");

        horizontalLayout->addWidget(eqPresetComboBox);

        enableEqCheckBox = new QCheckBox(SuperEqView);
        enableEqCheckBox->setObjectName("enableEqCheckBox");

        horizontalLayout->addWidget(enableEqCheckBox);

        resetButton = new QPushButton(SuperEqView);
        resetButton->setObjectName("resetButton");

        horizontalLayout->addWidget(resetButton);

        saveButton = new QPushButton(SuperEqView);
        saveButton->setObjectName("saveButton");

        horizontalLayout->addWidget(saveButton);

        deleteButton = new QPushButton(SuperEqView);
        deleteButton->setObjectName("deleteButton");

        horizontalLayout->addWidget(deleteButton);

        horizontalLayout->setStretch(1, 1);

        verticalLayout_13->addLayout(horizontalLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        verticalLayout_17 = new QVBoxLayout();
        verticalLayout_17->setSpacing(0);
        verticalLayout_17->setObjectName("verticalLayout_17");
        verticalLayout_17->setContentsMargins(1, -1, -1, -1);
        band10DbLabel = new QLabel(SuperEqView);
        band10DbLabel->setObjectName("band10DbLabel");
        band10DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_17->addWidget(band10DbLabel);

        band10Slider = new DoubleSlider(SuperEqView);
        band10Slider->setObjectName("band10Slider");
        band10Slider->setMinimum(-150);
        band10Slider->setMaximum(150);
        band10Slider->setOrientation(Qt::Vertical);
        band10Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_17->addWidget(band10Slider);

        band10FeqLabel = new QLabel(SuperEqView);
        band10FeqLabel->setObjectName("band10FeqLabel");
        band10FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_17->addWidget(band10FeqLabel);


        gridLayout->addLayout(verticalLayout_17, 0, 9, 1, 1);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(1, -1, -1, -1);
        band7DbLabel = new QLabel(SuperEqView);
        band7DbLabel->setObjectName("band7DbLabel");
        band7DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_4->addWidget(band7DbLabel);

        band7Slider = new DoubleSlider(SuperEqView);
        band7Slider->setObjectName("band7Slider");
        band7Slider->setMinimum(-150);
        band7Slider->setMaximum(150);
        band7Slider->setOrientation(Qt::Vertical);
        band7Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_4->addWidget(band7Slider);

        band7FeqLabel = new QLabel(SuperEqView);
        band7FeqLabel->setObjectName("band7FeqLabel");
        band7FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_4->addWidget(band7FeqLabel);


        gridLayout->addLayout(verticalLayout_4, 0, 6, 1, 1);

        verticalLayout_14 = new QVBoxLayout();
        verticalLayout_14->setSpacing(0);
        verticalLayout_14->setObjectName("verticalLayout_14");
        verticalLayout_14->setContentsMargins(1, -1, -1, -1);
        band6DbLabel = new QLabel(SuperEqView);
        band6DbLabel->setObjectName("band6DbLabel");
        band6DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_14->addWidget(band6DbLabel);

        band6Slider = new DoubleSlider(SuperEqView);
        band6Slider->setObjectName("band6Slider");
        band6Slider->setMinimum(-150);
        band6Slider->setMaximum(150);
        band6Slider->setOrientation(Qt::Vertical);
        band6Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_14->addWidget(band6Slider);

        band6FeqLabel = new QLabel(SuperEqView);
        band6FeqLabel->setObjectName("band6FeqLabel");
        band6FeqLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_14->addWidget(band6FeqLabel);


        gridLayout->addLayout(verticalLayout_14, 0, 5, 1, 1);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setContentsMargins(1, -1, -1, -1);
        band5DbLabel = new QLabel(SuperEqView);
        band5DbLabel->setObjectName("band5DbLabel");
        band5DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_3->addWidget(band5DbLabel);

        band5Slider = new DoubleSlider(SuperEqView);
        band5Slider->setObjectName("band5Slider");
        band5Slider->setMinimum(-150);
        band5Slider->setMaximum(150);
        band5Slider->setOrientation(Qt::Vertical);
        band5Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_3->addWidget(band5Slider);

        band5FeqLabel = new QLabel(SuperEqView);
        band5FeqLabel->setObjectName("band5FeqLabel");
        band5FeqLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(band5FeqLabel);


        gridLayout->addLayout(verticalLayout_3, 0, 4, 1, 1);

        verticalLayout_18 = new QVBoxLayout();
        verticalLayout_18->setSpacing(0);
        verticalLayout_18->setObjectName("verticalLayout_18");
        verticalLayout_18->setContentsMargins(1, -1, -1, -1);
        band12DbLabel = new QLabel(SuperEqView);
        band12DbLabel->setObjectName("band12DbLabel");
        band12DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_18->addWidget(band12DbLabel);

        band12Slider = new DoubleSlider(SuperEqView);
        band12Slider->setObjectName("band12Slider");
        band12Slider->setMinimum(-150);
        band12Slider->setMaximum(150);
        band12Slider->setOrientation(Qt::Vertical);
        band12Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_18->addWidget(band12Slider);

        band12FeqLabel = new QLabel(SuperEqView);
        band12FeqLabel->setObjectName("band12FeqLabel");
        band12FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_18->addWidget(band12FeqLabel);


        gridLayout->addLayout(verticalLayout_18, 0, 11, 1, 1);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName("verticalLayout_7");
        verticalLayout_7->setContentsMargins(1, -1, -1, -1);
        band13DbLabel = new QLabel(SuperEqView);
        band13DbLabel->setObjectName("band13DbLabel");
        band13DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_7->addWidget(band13DbLabel);

        band13Slider = new DoubleSlider(SuperEqView);
        band13Slider->setObjectName("band13Slider");
        band13Slider->setMinimum(-150);
        band13Slider->setMaximum(150);
        band13Slider->setOrientation(Qt::Vertical);
        band13Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_7->addWidget(band13Slider);

        band13FeqLabel = new QLabel(SuperEqView);
        band13FeqLabel->setObjectName("band13FeqLabel");
        band13FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_7->addWidget(band13FeqLabel);


        gridLayout->addLayout(verticalLayout_7, 0, 12, 1, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(1, -1, -1, -1);
        band3DbLabel = new QLabel(SuperEqView);
        band3DbLabel->setObjectName("band3DbLabel");
        band3DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_2->addWidget(band3DbLabel);

        band3Slider = new DoubleSlider(SuperEqView);
        band3Slider->setObjectName("band3Slider");
        band3Slider->setMinimum(-150);
        band3Slider->setMaximum(150);
        band3Slider->setOrientation(Qt::Vertical);
        band3Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_2->addWidget(band3Slider);

        band3FeqLabel = new QLabel(SuperEqView);
        band3FeqLabel->setObjectName("band3FeqLabel");
        band3FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_2->addWidget(band3FeqLabel);


        gridLayout->addLayout(verticalLayout_2, 0, 2, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(1, -1, -1, -1);
        band1DbLabel = new QLabel(SuperEqView);
        band1DbLabel->setObjectName("band1DbLabel");
        band1DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout->addWidget(band1DbLabel);

        band1Slider = new DoubleSlider(SuperEqView);
        band1Slider->setObjectName("band1Slider");
        band1Slider->setMinimum(-150);
        band1Slider->setMaximum(150);
        band1Slider->setOrientation(Qt::Vertical);
        band1Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout->addWidget(band1Slider);

        band1FeqLabel = new QLabel(SuperEqView);
        band1FeqLabel->setObjectName("band1FeqLabel");
        band1FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout->addWidget(band1FeqLabel);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);

        verticalLayout_12 = new QVBoxLayout();
        verticalLayout_12->setSpacing(0);
        verticalLayout_12->setObjectName("verticalLayout_12");
        verticalLayout_12->setContentsMargins(1, -1, -1, -1);
        band4DbLabel = new QLabel(SuperEqView);
        band4DbLabel->setObjectName("band4DbLabel");
        band4DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_12->addWidget(band4DbLabel);

        band4Slider = new DoubleSlider(SuperEqView);
        band4Slider->setObjectName("band4Slider");
        band4Slider->setMinimum(-150);
        band4Slider->setMaximum(150);
        band4Slider->setOrientation(Qt::Vertical);
        band4Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_12->addWidget(band4Slider);

        band4FeqLabel = new QLabel(SuperEqView);
        band4FeqLabel->setObjectName("band4FeqLabel");
        band4FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_12->addWidget(band4FeqLabel);


        gridLayout->addLayout(verticalLayout_12, 0, 3, 1, 1);

        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setSpacing(0);
        verticalLayout_9->setObjectName("verticalLayout_9");
        verticalLayout_9->setContentsMargins(1, -1, -1, -1);
        band17DbLabel = new QLabel(SuperEqView);
        band17DbLabel->setObjectName("band17DbLabel");
        band17DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_9->addWidget(band17DbLabel);

        band17Slider = new DoubleSlider(SuperEqView);
        band17Slider->setObjectName("band17Slider");
        band17Slider->setMinimum(-150);
        band17Slider->setMaximum(150);
        band17Slider->setOrientation(Qt::Vertical);
        band17Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_9->addWidget(band17Slider);

        band17FeqLabel = new QLabel(SuperEqView);
        band17FeqLabel->setObjectName("band17FeqLabel");
        band17FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_9->addWidget(band17FeqLabel);


        gridLayout->addLayout(verticalLayout_9, 0, 16, 1, 1);

        verticalLayout_16 = new QVBoxLayout();
        verticalLayout_16->setSpacing(0);
        verticalLayout_16->setObjectName("verticalLayout_16");
        verticalLayout_16->setContentsMargins(1, -1, -1, -1);
        band8DbLabel = new QLabel(SuperEqView);
        band8DbLabel->setObjectName("band8DbLabel");
        band8DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_16->addWidget(band8DbLabel);

        band8Slider = new DoubleSlider(SuperEqView);
        band8Slider->setObjectName("band8Slider");
        band8Slider->setMinimum(-150);
        band8Slider->setMaximum(150);
        band8Slider->setOrientation(Qt::Vertical);
        band8Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_16->addWidget(band8Slider);

        band8FeqLabel = new QLabel(SuperEqView);
        band8FeqLabel->setObjectName("band8FeqLabel");
        band8FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_16->addWidget(band8FeqLabel);


        gridLayout->addLayout(verticalLayout_16, 0, 7, 1, 1);

        verticalLayout_20 = new QVBoxLayout();
        verticalLayout_20->setSpacing(0);
        verticalLayout_20->setObjectName("verticalLayout_20");
        verticalLayout_20->setContentsMargins(1, -1, -1, -1);
        band16DbLabel = new QLabel(SuperEqView);
        band16DbLabel->setObjectName("band16DbLabel");
        band16DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_20->addWidget(band16DbLabel);

        band16Slider = new DoubleSlider(SuperEqView);
        band16Slider->setObjectName("band16Slider");
        band16Slider->setMinimum(-150);
        band16Slider->setMaximum(150);
        band16Slider->setOrientation(Qt::Vertical);
        band16Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_20->addWidget(band16Slider);

        band16FeqLabel = new QLabel(SuperEqView);
        band16FeqLabel->setObjectName("band16FeqLabel");
        band16FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_20->addWidget(band16FeqLabel);


        gridLayout->addLayout(verticalLayout_20, 0, 15, 1, 1);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setObjectName("verticalLayout_8");
        verticalLayout_8->setContentsMargins(1, -1, -1, -1);
        band15DbLabel = new QLabel(SuperEqView);
        band15DbLabel->setObjectName("band15DbLabel");
        band15DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_8->addWidget(band15DbLabel);

        band15Slider = new DoubleSlider(SuperEqView);
        band15Slider->setObjectName("band15Slider");
        band15Slider->setMinimum(-150);
        band15Slider->setMaximum(150);
        band15Slider->setOrientation(Qt::Vertical);
        band15Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_8->addWidget(band15Slider);

        band15FeqLabel = new QLabel(SuperEqView);
        band15FeqLabel->setObjectName("band15FeqLabel");
        band15FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_8->addWidget(band15FeqLabel);


        gridLayout->addLayout(verticalLayout_8, 0, 14, 1, 1);

        verticalLayout_10 = new QVBoxLayout();
        verticalLayout_10->setSpacing(0);
        verticalLayout_10->setObjectName("verticalLayout_10");
        verticalLayout_10->setContentsMargins(1, -1, 0, -1);
        band18DbLabel = new QLabel(SuperEqView);
        band18DbLabel->setObjectName("band18DbLabel");
        band18DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_10->addWidget(band18DbLabel);

        band18Slider = new DoubleSlider(SuperEqView);
        band18Slider->setObjectName("band18Slider");
        band18Slider->setMinimum(-150);
        band18Slider->setMaximum(150);
        band18Slider->setOrientation(Qt::Vertical);
        band18Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_10->addWidget(band18Slider);

        band18FeqLabel = new QLabel(SuperEqView);
        band18FeqLabel->setObjectName("band18FeqLabel");
        band18FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_10->addWidget(band18FeqLabel);


        gridLayout->addLayout(verticalLayout_10, 0, 17, 1, 1);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(0);
        verticalLayout_5->setObjectName("verticalLayout_5");
        verticalLayout_5->setContentsMargins(1, -1, -1, -1);
        band9DbLabel = new QLabel(SuperEqView);
        band9DbLabel->setObjectName("band9DbLabel");
        band9DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_5->addWidget(band9DbLabel);

        band9Slider = new DoubleSlider(SuperEqView);
        band9Slider->setObjectName("band9Slider");
        band9Slider->setMinimum(-150);
        band9Slider->setMaximum(150);
        band9Slider->setOrientation(Qt::Vertical);
        band9Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_5->addWidget(band9Slider);

        band9FeqLabel = new QLabel(SuperEqView);
        band9FeqLabel->setObjectName("band9FeqLabel");
        band9FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_5->addWidget(band9FeqLabel);


        gridLayout->addLayout(verticalLayout_5, 0, 8, 1, 1);

        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName("verticalLayout_6");
        verticalLayout_6->setContentsMargins(1, -1, -1, -1);
        band11DbLabel = new QLabel(SuperEqView);
        band11DbLabel->setObjectName("band11DbLabel");
        band11DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_6->addWidget(band11DbLabel);

        band11Slider = new DoubleSlider(SuperEqView);
        band11Slider->setObjectName("band11Slider");
        band11Slider->setMinimum(-150);
        band11Slider->setMaximum(150);
        band11Slider->setOrientation(Qt::Vertical);
        band11Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_6->addWidget(band11Slider);

        band11FeqLabel = new QLabel(SuperEqView);
        band11FeqLabel->setObjectName("band11FeqLabel");
        band11FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_6->addWidget(band11FeqLabel);


        gridLayout->addLayout(verticalLayout_6, 0, 10, 1, 1);

        verticalLayout_11 = new QVBoxLayout();
        verticalLayout_11->setSpacing(0);
        verticalLayout_11->setObjectName("verticalLayout_11");
        verticalLayout_11->setContentsMargins(1, -1, -1, -1);
        band2DbLabel = new QLabel(SuperEqView);
        band2DbLabel->setObjectName("band2DbLabel");
        band2DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_11->addWidget(band2DbLabel);

        band2Slider = new DoubleSlider(SuperEqView);
        band2Slider->setObjectName("band2Slider");
        band2Slider->setMinimum(-150);
        band2Slider->setMaximum(150);
        band2Slider->setOrientation(Qt::Vertical);
        band2Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_11->addWidget(band2Slider);

        band2FeqLabel = new QLabel(SuperEqView);
        band2FeqLabel->setObjectName("band2FeqLabel");
        band2FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_11->addWidget(band2FeqLabel);


        gridLayout->addLayout(verticalLayout_11, 0, 1, 1, 1);

        verticalLayout_19 = new QVBoxLayout();
        verticalLayout_19->setSpacing(0);
        verticalLayout_19->setObjectName("verticalLayout_19");
        verticalLayout_19->setContentsMargins(1, -1, -1, -1);
        band14DbLabel = new QLabel(SuperEqView);
        band14DbLabel->setObjectName("band14DbLabel");
        band14DbLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_19->addWidget(band14DbLabel);

        band14Slider = new DoubleSlider(SuperEqView);
        band14Slider->setObjectName("band14Slider");
        band14Slider->setMinimum(-150);
        band14Slider->setMaximum(150);
        band14Slider->setOrientation(Qt::Vertical);
        band14Slider->setTickPosition(QSlider::TicksAbove);

        verticalLayout_19->addWidget(band14Slider);

        band14FeqLabel = new QLabel(SuperEqView);
        band14FeqLabel->setObjectName("band14FeqLabel");
        band14FeqLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        verticalLayout_19->addWidget(band14FeqLabel);


        gridLayout->addLayout(verticalLayout_19, 0, 13, 1, 1);


        horizontalLayout_3->addLayout(gridLayout);

        horizontalLayout_3->setStretch(0, 1);

        verticalLayout_13->addLayout(horizontalLayout_3);

        verticalLayout_13->setStretch(1, 1);

        retranslateUi(SuperEqView);

        QMetaObject::connectSlotsByName(SuperEqView);
    } // setupUi

    void retranslateUi(QFrame *SuperEqView)
    {
        SuperEqView->setWindowTitle(QCoreApplication::translate("SuperEqView", "Equalizer", nullptr));
        label_23->setText(QCoreApplication::translate("SuperEqView", "Preset:", nullptr));
        enableEqCheckBox->setText(QCoreApplication::translate("SuperEqView", "Eanble", nullptr));
        resetButton->setText(QCoreApplication::translate("SuperEqView", "Reset", nullptr));
        saveButton->setText(QCoreApplication::translate("SuperEqView", "Save", nullptr));
        deleteButton->setText(QCoreApplication::translate("SuperEqView", "Delete", nullptr));
        band10DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band10FeqLabel->setText(QCoreApplication::translate("SuperEqView", "1.2kHz", nullptr));
        band7DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band7FeqLabel->setText(QCoreApplication::translate("SuperEqView", "440Hz", nullptr));
        band6DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band6FeqLabel->setText(QCoreApplication::translate("SuperEqView", "311Hz", nullptr));
        band5DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band5FeqLabel->setText(QCoreApplication::translate("SuperEqView", "220Hz", nullptr));
        band12DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band12FeqLabel->setText(QCoreApplication::translate("SuperEqView", "2.5kHz", nullptr));
        band13DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band13FeqLabel->setText(QCoreApplication::translate("SuperEqView", "3.5kHz", nullptr));
        band3DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band3FeqLabel->setText(QCoreApplication::translate("SuperEqView", "110Hz", nullptr));
        band1DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band1FeqLabel->setText(QCoreApplication::translate("SuperEqView", "55Hz", nullptr));
        band4DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band4FeqLabel->setText(QCoreApplication::translate("SuperEqView", "156Hz", nullptr));
        band17DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band17FeqLabel->setText(QCoreApplication::translate("SuperEqView", "14kHz", nullptr));
        band8DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band8FeqLabel->setText(QCoreApplication::translate("SuperEqView", "622Hz", nullptr));
        band16DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band16FeqLabel->setText(QCoreApplication::translate("SuperEqView", "10kHz", nullptr));
        band15DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band15FeqLabel->setText(QCoreApplication::translate("SuperEqView", "7kHz", nullptr));
        band18DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band18FeqLabel->setText(QCoreApplication::translate("SuperEqView", "20KHz", nullptr));
        band9DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band9FeqLabel->setText(QCoreApplication::translate("SuperEqView", "880Hz", nullptr));
        band11DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band11FeqLabel->setText(QCoreApplication::translate("SuperEqView", "1.8kHz", nullptr));
        band2DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band2FeqLabel->setText(QCoreApplication::translate("SuperEqView", "77Hz", nullptr));
        band14DbLabel->setText(QCoreApplication::translate("SuperEqView", "0db", nullptr));
        band14FeqLabel->setText(QCoreApplication::translate("SuperEqView", "5kHz", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SuperEqView: public Ui_SuperEqView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SUPEREQDIALOG_H
