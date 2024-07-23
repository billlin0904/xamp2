/********************************************************************************
** Form generated from reading UI file 'preferencedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PREFERENCEDIALOG_H
#define UI_PREFERENCEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "widget/seekslider.h"

QT_BEGIN_NAMESPACE

class Ui_PreferenceDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QTreeWidget *preferenceTreeWidget;
    QStackedWidget *stackedWidget;
    QWidget *playbackPage;
    QVBoxLayout *verticalLayout_17;
    QVBoxLayout *verticalLayout_15;
    QLabel *lbLang;
    QComboBox *langCombo;
    QSpacerItem *verticalSpacer_7;
    QVBoxLayout *verticalLayout_16;
    QLabel *lbThemeMode;
    QRadioButton *lightRadioButton;
    QRadioButton *darkRadioButton;
    QSpacerItem *verticalSpacer_5;
    QVBoxLayout *verticalLayout_5;
    QLabel *lbReplayGameMode;
    QComboBox *replayGainModeCombo;
    QSpacerItem *verticalSpacer;
    QWidget *dspManagerPage;
    QStackedWidget *resamplerStackedWidget;
    QWidget *noneResamplerPage;
    QWidget *soxrResamplerPage;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout_12;
    QLabel *lbResamplerSettings;
    QComboBox *soxrSettingCombo;
    QHBoxLayout *horizontalLayout_10;
    QPushButton *newSoxrSettingBtn;
    QPushButton *saveSoxrSettingBtn;
    QPushButton *deleteSoxrSettingBtn;
    QSpacerItem *verticalSpacer_13;
    QHBoxLayout *horizontalLayout_9;
    QHBoxLayout *horizontalLayout_8;
    QHBoxLayout *horizontalLayout_6;
    QLabel *lbTargetSampleRate;
    QComboBox *soxrTargetSampleRateComboBox;
    QLabel *lbHz;
    QHBoxLayout *horizontalLayout_7;
    QSpacerItem *horizontalSpacer_3;
    QLabel *lbQuality;
    QComboBox *soxrResampleQualityComboBox;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *verticalSpacer_9;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_5;
    QLabel *lbRollOffLevel;
    QComboBox *rollOffLevelComboBox;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *verticalSpacer_10;
    QHBoxLayout *horizontalLayout_11;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_7;
    SeekSlider *soxrPassbandSlider;
    QLabel *soxrPassbandValue;
    QSpacerItem *verticalSpacer_11;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_8;
    SeekSlider *soxrPhaseSlider;
    QLabel *soxrPhaseValue;
    QSpacerItem *verticalSpacer_4;
    QWidget *srcResamplerPage;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_2;
    QLabel *lbSrcTargetSampleRate;
    QComboBox *srcTargetSampleRateComboBox;
    QLabel *lbSrcHz;
    QSpacerItem *verticalSpacer_2;
    QWidget *r8brainResamplerPage;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QLabel *lbR8BrainTargetSampleRate;
    QComboBox *r8brainTargetSampleRateComboBox;
    QLabel *lbR8BrainHz;
    QSpacerItem *verticalSpacer_3;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout_27;
    QLabel *lbResampler;
    QComboBox *selectResamplerComboBox;
    QHBoxLayout *horizontalLayout_13;
    QPushButton *resetAllButton;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QFrame *PreferenceDialog)
    {
        if (PreferenceDialog->objectName().isEmpty())
            PreferenceDialog->setObjectName("PreferenceDialog");
        PreferenceDialog->resize(1024, 822);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PreferenceDialog->sizePolicy().hasHeightForWidth());
        PreferenceDialog->setSizePolicy(sizePolicy);
        PreferenceDialog->setFocusPolicy(Qt::NoFocus);
        verticalLayout = new QVBoxLayout(PreferenceDialog);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        preferenceTreeWidget = new QTreeWidget(PreferenceDialog);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        preferenceTreeWidget->setHeaderItem(__qtreewidgetitem);
        preferenceTreeWidget->setObjectName("preferenceTreeWidget");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(preferenceTreeWidget->sizePolicy().hasHeightForWidth());
        preferenceTreeWidget->setSizePolicy(sizePolicy1);
        preferenceTreeWidget->setMinimumSize(QSize(220, 0));
        preferenceTreeWidget->setMaximumSize(QSize(220, 16777215));

        horizontalLayout->addWidget(preferenceTreeWidget);

        stackedWidget = new QStackedWidget(PreferenceDialog);
        stackedWidget->setObjectName("stackedWidget");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(2);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(stackedWidget->sizePolicy().hasHeightForWidth());
        stackedWidget->setSizePolicy(sizePolicy2);
        playbackPage = new QWidget();
        playbackPage->setObjectName("playbackPage");
        verticalLayout_17 = new QVBoxLayout(playbackPage);
        verticalLayout_17->setObjectName("verticalLayout_17");
        verticalLayout_15 = new QVBoxLayout();
        verticalLayout_15->setObjectName("verticalLayout_15");
        lbLang = new QLabel(playbackPage);
        lbLang->setObjectName("lbLang");
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        font.setKerning(false);
        lbLang->setFont(font);

        verticalLayout_15->addWidget(lbLang);

        langCombo = new QComboBox(playbackPage);
        langCombo->setObjectName("langCombo");
        langCombo->setMaximumSize(QSize(150, 16777215));
        QFont font1;
        font1.setKerning(false);
        langCombo->setFont(font1);

        verticalLayout_15->addWidget(langCombo);


        verticalLayout_17->addLayout(verticalLayout_15);

        verticalSpacer_7 = new QSpacerItem(20, 30, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_17->addItem(verticalSpacer_7);

        verticalLayout_16 = new QVBoxLayout();
        verticalLayout_16->setObjectName("verticalLayout_16");
        lbThemeMode = new QLabel(playbackPage);
        lbThemeMode->setObjectName("lbThemeMode");
        lbThemeMode->setFont(font);

        verticalLayout_16->addWidget(lbThemeMode);

        lightRadioButton = new QRadioButton(playbackPage);
        lightRadioButton->setObjectName("lightRadioButton");

        verticalLayout_16->addWidget(lightRadioButton);

        darkRadioButton = new QRadioButton(playbackPage);
        darkRadioButton->setObjectName("darkRadioButton");

        verticalLayout_16->addWidget(darkRadioButton);


        verticalLayout_17->addLayout(verticalLayout_16);

        verticalSpacer_5 = new QSpacerItem(20, 30, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_17->addItem(verticalSpacer_5);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName("verticalLayout_5");
        lbReplayGameMode = new QLabel(playbackPage);
        lbReplayGameMode->setObjectName("lbReplayGameMode");
        lbReplayGameMode->setMaximumSize(QSize(16777215, 16777215));
        lbReplayGameMode->setFont(font);

        verticalLayout_5->addWidget(lbReplayGameMode);

        replayGainModeCombo = new QComboBox(playbackPage);
        replayGainModeCombo->addItem(QString());
        replayGainModeCombo->addItem(QString());
        replayGainModeCombo->addItem(QString());
        replayGainModeCombo->setObjectName("replayGainModeCombo");
        replayGainModeCombo->setMaximumSize(QSize(150, 16777215));
        replayGainModeCombo->setFont(font1);

        verticalLayout_5->addWidget(replayGainModeCombo);


        verticalLayout_17->addLayout(verticalLayout_5);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_17->addItem(verticalSpacer);

        stackedWidget->addWidget(playbackPage);
        dspManagerPage = new QWidget();
        dspManagerPage->setObjectName("dspManagerPage");
        resamplerStackedWidget = new QStackedWidget(dspManagerPage);
        resamplerStackedWidget->setObjectName("resamplerStackedWidget");
        resamplerStackedWidget->setGeometry(QRect(10, 50, 591, 701));
        noneResamplerPage = new QWidget();
        noneResamplerPage->setObjectName("noneResamplerPage");
        resamplerStackedWidget->addWidget(noneResamplerPage);
        soxrResamplerPage = new QWidget();
        soxrResamplerPage->setObjectName("soxrResamplerPage");
        verticalLayout_4 = new QVBoxLayout(soxrResamplerPage);
        verticalLayout_4->setSpacing(6);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(0, 6, 0, 0);
        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName("horizontalLayout_12");
        lbResamplerSettings = new QLabel(soxrResamplerPage);
        lbResamplerSettings->setObjectName("lbResamplerSettings");
        lbResamplerSettings->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_12->addWidget(lbResamplerSettings);

        soxrSettingCombo = new QComboBox(soxrResamplerPage);
        soxrSettingCombo->setObjectName("soxrSettingCombo");

        horizontalLayout_12->addWidget(soxrSettingCombo);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName("horizontalLayout_10");
        newSoxrSettingBtn = new QPushButton(soxrResamplerPage);
        newSoxrSettingBtn->setObjectName("newSoxrSettingBtn");

        horizontalLayout_10->addWidget(newSoxrSettingBtn);

        saveSoxrSettingBtn = new QPushButton(soxrResamplerPage);
        saveSoxrSettingBtn->setObjectName("saveSoxrSettingBtn");

        horizontalLayout_10->addWidget(saveSoxrSettingBtn);

        deleteSoxrSettingBtn = new QPushButton(soxrResamplerPage);
        deleteSoxrSettingBtn->setObjectName("deleteSoxrSettingBtn");

        horizontalLayout_10->addWidget(deleteSoxrSettingBtn);


        horizontalLayout_12->addLayout(horizontalLayout_10);


        verticalLayout_4->addLayout(horizontalLayout_12);

        verticalSpacer_13 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_4->addItem(verticalSpacer_13);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        lbTargetSampleRate = new QLabel(soxrResamplerPage);
        lbTargetSampleRate->setObjectName("lbTargetSampleRate");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy3.setHorizontalStretch(1);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(lbTargetSampleRate->sizePolicy().hasHeightForWidth());
        lbTargetSampleRate->setSizePolicy(sizePolicy3);

        horizontalLayout_6->addWidget(lbTargetSampleRate);

        soxrTargetSampleRateComboBox = new QComboBox(soxrResamplerPage);
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->addItem(QString());
        soxrTargetSampleRateComboBox->setObjectName("soxrTargetSampleRateComboBox");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(soxrTargetSampleRateComboBox->sizePolicy().hasHeightForWidth());
        soxrTargetSampleRateComboBox->setSizePolicy(sizePolicy4);

        horizontalLayout_6->addWidget(soxrTargetSampleRateComboBox);

        lbHz = new QLabel(soxrResamplerPage);
        lbHz->setObjectName("lbHz");

        horizontalLayout_6->addWidget(lbHz);


        horizontalLayout_8->addLayout(horizontalLayout_6);


        horizontalLayout_9->addLayout(horizontalLayout_8);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(1);
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        horizontalSpacer_3 = new QSpacerItem(50, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_3);

        lbQuality = new QLabel(soxrResamplerPage);
        lbQuality->setObjectName("lbQuality");
        QSizePolicy sizePolicy5(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(lbQuality->sizePolicy().hasHeightForWidth());
        lbQuality->setSizePolicy(sizePolicy5);

        horizontalLayout_7->addWidget(lbQuality);

        soxrResampleQualityComboBox = new QComboBox(soxrResamplerPage);
        soxrResampleQualityComboBox->addItem(QString());
        soxrResampleQualityComboBox->addItem(QString());
        soxrResampleQualityComboBox->addItem(QString());
        soxrResampleQualityComboBox->addItem(QString());
        soxrResampleQualityComboBox->addItem(QString());
        soxrResampleQualityComboBox->setObjectName("soxrResampleQualityComboBox");
        sizePolicy4.setHeightForWidth(soxrResampleQualityComboBox->sizePolicy().hasHeightForWidth());
        soxrResampleQualityComboBox->setSizePolicy(sizePolicy4);

        horizontalLayout_7->addWidget(soxrResampleQualityComboBox);

        horizontalSpacer = new QSpacerItem(50, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer);


        horizontalLayout_9->addLayout(horizontalLayout_7);


        verticalLayout_4->addLayout(horizontalLayout_9);

        verticalSpacer_9 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_4->addItem(verticalSpacer_9);

        groupBox = new QGroupBox(soxrResamplerPage);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy6(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(1);
        sizePolicy6.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy6);
        verticalLayout_3 = new QVBoxLayout(groupBox);
        verticalLayout_3->setObjectName("verticalLayout_3");
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        lbRollOffLevel = new QLabel(groupBox);
        lbRollOffLevel->setObjectName("lbRollOffLevel");
        sizePolicy5.setHeightForWidth(lbRollOffLevel->sizePolicy().hasHeightForWidth());
        lbRollOffLevel->setSizePolicy(sizePolicy5);

        horizontalLayout_5->addWidget(lbRollOffLevel);

        rollOffLevelComboBox = new QComboBox(groupBox);
        rollOffLevelComboBox->addItem(QString());
        rollOffLevelComboBox->addItem(QString());
        rollOffLevelComboBox->addItem(QString());
        rollOffLevelComboBox->setObjectName("rollOffLevelComboBox");

        horizontalLayout_5->addWidget(rollOffLevelComboBox);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_7);

        horizontalLayout_5->setStretch(1, 1);
        horizontalLayout_5->setStretch(2, 1);

        verticalLayout_3->addLayout(horizontalLayout_5);


        verticalLayout_4->addWidget(groupBox);

        verticalSpacer_10 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_4->addItem(verticalSpacer_10);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setObjectName("horizontalLayout_11");

        verticalLayout_4->addLayout(horizontalLayout_11);

        groupBox_2 = new QGroupBox(soxrResamplerPage);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setMinimumSize(QSize(0, 50));
        verticalLayout_7 = new QVBoxLayout(groupBox_2);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName("verticalLayout_7");
        soxrPassbandSlider = new SeekSlider(groupBox_2);
        soxrPassbandSlider->setObjectName("soxrPassbandSlider");
        soxrPassbandSlider->setMinimum(1);
        soxrPassbandSlider->setMaximum(99);
        soxrPassbandSlider->setSingleStep(1);
        soxrPassbandSlider->setPageStep(1);
        soxrPassbandSlider->setSliderPosition(90);
        soxrPassbandSlider->setOrientation(Qt::Horizontal);
        soxrPassbandSlider->setTickPosition(QSlider::TicksBelow);
        soxrPassbandSlider->setTickInterval(5);

        verticalLayout_7->addWidget(soxrPassbandSlider);

        soxrPassbandValue = new QLabel(groupBox_2);
        soxrPassbandValue->setObjectName("soxrPassbandValue");
        soxrPassbandValue->setAlignment(Qt::AlignCenter);

        verticalLayout_7->addWidget(soxrPassbandValue);


        verticalLayout_4->addWidget(groupBox_2);

        verticalSpacer_11 = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayout_4->addItem(verticalSpacer_11);

        groupBox_3 = new QGroupBox(soxrResamplerPage);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setMinimumSize(QSize(0, 50));
        verticalLayout_8 = new QVBoxLayout(groupBox_3);
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setObjectName("verticalLayout_8");
        soxrPhaseSlider = new SeekSlider(groupBox_3);
        soxrPhaseSlider->setObjectName("soxrPhaseSlider");
        soxrPhaseSlider->setMinimum(0);
        soxrPhaseSlider->setMaximum(50);
        soxrPhaseSlider->setSliderPosition(45);
        soxrPhaseSlider->setOrientation(Qt::Horizontal);
        soxrPhaseSlider->setTickPosition(QSlider::TicksBelow);
        soxrPhaseSlider->setTickInterval(1);

        verticalLayout_8->addWidget(soxrPhaseSlider);

        soxrPhaseValue = new QLabel(groupBox_3);
        soxrPhaseValue->setObjectName("soxrPhaseValue");
        soxrPhaseValue->setAlignment(Qt::AlignCenter);

        verticalLayout_8->addWidget(soxrPhaseValue);


        verticalLayout_4->addWidget(groupBox_3);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_4);

        resamplerStackedWidget->addWidget(soxrResamplerPage);
        srcResamplerPage = new QWidget();
        srcResamplerPage->setObjectName("srcResamplerPage");
        verticalLayout_6 = new QVBoxLayout(srcResamplerPage);
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName("verticalLayout_6");
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        lbSrcTargetSampleRate = new QLabel(srcResamplerPage);
        lbSrcTargetSampleRate->setObjectName("lbSrcTargetSampleRate");
        lbSrcTargetSampleRate->setMaximumSize(QSize(150, 16777215));

        horizontalLayout_2->addWidget(lbSrcTargetSampleRate);

        srcTargetSampleRateComboBox = new QComboBox(srcResamplerPage);
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->addItem(QString());
        srcTargetSampleRateComboBox->setObjectName("srcTargetSampleRateComboBox");
        srcTargetSampleRateComboBox->setMaximumSize(QSize(150, 16777215));

        horizontalLayout_2->addWidget(srcTargetSampleRateComboBox);

        lbSrcHz = new QLabel(srcResamplerPage);
        lbSrcHz->setObjectName("lbSrcHz");

        horizontalLayout_2->addWidget(lbSrcHz);

        horizontalLayout_2->setStretch(1, 1);

        verticalLayout_6->addLayout(horizontalLayout_2);

        verticalSpacer_2 = new QSpacerItem(20, 675, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_2);

        resamplerStackedWidget->addWidget(srcResamplerPage);
        r8brainResamplerPage = new QWidget();
        r8brainResamplerPage->setObjectName("r8brainResamplerPage");
        verticalLayout_2 = new QVBoxLayout(r8brainResamplerPage);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        lbR8BrainTargetSampleRate = new QLabel(r8brainResamplerPage);
        lbR8BrainTargetSampleRate->setObjectName("lbR8BrainTargetSampleRate");
        lbR8BrainTargetSampleRate->setMaximumSize(QSize(150, 16777215));

        horizontalLayout_4->addWidget(lbR8BrainTargetSampleRate);

        r8brainTargetSampleRateComboBox = new QComboBox(r8brainResamplerPage);
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->addItem(QString());
        r8brainTargetSampleRateComboBox->setObjectName("r8brainTargetSampleRateComboBox");
        r8brainTargetSampleRateComboBox->setMaximumSize(QSize(150, 16777215));

        horizontalLayout_4->addWidget(r8brainTargetSampleRateComboBox);

        lbR8BrainHz = new QLabel(r8brainResamplerPage);
        lbR8BrainHz->setObjectName("lbR8BrainHz");

        horizontalLayout_4->addWidget(lbR8BrainHz);

        horizontalLayout_4->setStretch(1, 1);

        verticalLayout_2->addLayout(horizontalLayout_4);

        verticalSpacer_3 = new QSpacerItem(20, 306, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);

        resamplerStackedWidget->addWidget(r8brainResamplerPage);
        layoutWidget = new QWidget(dspManagerPage);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(0, 0, 381, 41));
        horizontalLayout_27 = new QHBoxLayout(layoutWidget);
        horizontalLayout_27->setObjectName("horizontalLayout_27");
        horizontalLayout_27->setContentsMargins(0, 0, 0, 0);
        lbResampler = new QLabel(layoutWidget);
        lbResampler->setObjectName("lbResampler");
        sizePolicy3.setHeightForWidth(lbResampler->sizePolicy().hasHeightForWidth());
        lbResampler->setSizePolicy(sizePolicy3);

        horizontalLayout_27->addWidget(lbResampler);

        selectResamplerComboBox = new QComboBox(layoutWidget);
        selectResamplerComboBox->addItem(QString());
        selectResamplerComboBox->addItem(QString());
        selectResamplerComboBox->addItem(QString());
        selectResamplerComboBox->addItem(QString());
        selectResamplerComboBox->setObjectName("selectResamplerComboBox");
        QSizePolicy sizePolicy7(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy7.setHorizontalStretch(1);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(selectResamplerComboBox->sizePolicy().hasHeightForWidth());
        selectResamplerComboBox->setSizePolicy(sizePolicy7);

        horizontalLayout_27->addWidget(selectResamplerComboBox);

        stackedWidget->addWidget(dspManagerPage);

        horizontalLayout->addWidget(stackedWidget);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName("horizontalLayout_13");
        resetAllButton = new QPushButton(PreferenceDialog);
        resetAllButton->setObjectName("resetAllButton");

        horizontalLayout_13->addWidget(resetAllButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_13);


        retranslateUi(PreferenceDialog);

        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(PreferenceDialog);
    } // setupUi

    void retranslateUi(QFrame *PreferenceDialog)
    {
        PreferenceDialog->setWindowTitle(QCoreApplication::translate("PreferenceDialog", "Preference", nullptr));
        lbLang->setText(QCoreApplication::translate("PreferenceDialog", "Language:", nullptr));
        lbThemeMode->setText(QCoreApplication::translate("PreferenceDialog", "Theme Mode", nullptr));
        lightRadioButton->setText(QCoreApplication::translate("PreferenceDialog", "Light", nullptr));
        darkRadioButton->setText(QCoreApplication::translate("PreferenceDialog", "Dark", nullptr));
        lbReplayGameMode->setText(QCoreApplication::translate("PreferenceDialog", "ReplayGain mode", nullptr));
        replayGainModeCombo->setItemText(0, QCoreApplication::translate("PreferenceDialog", "Album", nullptr));
        replayGainModeCombo->setItemText(1, QCoreApplication::translate("PreferenceDialog", "Track", nullptr));
        replayGainModeCombo->setItemText(2, QCoreApplication::translate("PreferenceDialog", "None", nullptr));

        lbResamplerSettings->setText(QCoreApplication::translate("PreferenceDialog", "Settings:", nullptr));
        newSoxrSettingBtn->setText(QCoreApplication::translate("PreferenceDialog", "New", nullptr));
        saveSoxrSettingBtn->setText(QCoreApplication::translate("PreferenceDialog", "Save", nullptr));
        deleteSoxrSettingBtn->setText(QCoreApplication::translate("PreferenceDialog", "Delete", nullptr));
        lbTargetSampleRate->setText(QCoreApplication::translate("PreferenceDialog", "Target samplerate:", nullptr));
        soxrTargetSampleRateComboBox->setItemText(0, QCoreApplication::translate("PreferenceDialog", "44100", nullptr));
        soxrTargetSampleRateComboBox->setItemText(1, QCoreApplication::translate("PreferenceDialog", "48000", nullptr));
        soxrTargetSampleRateComboBox->setItemText(2, QCoreApplication::translate("PreferenceDialog", "88200", nullptr));
        soxrTargetSampleRateComboBox->setItemText(3, QCoreApplication::translate("PreferenceDialog", "96000", nullptr));
        soxrTargetSampleRateComboBox->setItemText(4, QCoreApplication::translate("PreferenceDialog", "176400", nullptr));
        soxrTargetSampleRateComboBox->setItemText(5, QCoreApplication::translate("PreferenceDialog", "192000", nullptr));
        soxrTargetSampleRateComboBox->setItemText(6, QCoreApplication::translate("PreferenceDialog", "352800", nullptr));
        soxrTargetSampleRateComboBox->setItemText(7, QCoreApplication::translate("PreferenceDialog", "384000", nullptr));
        soxrTargetSampleRateComboBox->setItemText(8, QCoreApplication::translate("PreferenceDialog", "768000", nullptr));

        lbHz->setText(QCoreApplication::translate("PreferenceDialog", "Hz", nullptr));
        lbQuality->setText(QCoreApplication::translate("PreferenceDialog", "Quality:", nullptr));
        soxrResampleQualityComboBox->setItemText(0, QCoreApplication::translate("PreferenceDialog", "Low", nullptr));
        soxrResampleQualityComboBox->setItemText(1, QCoreApplication::translate("PreferenceDialog", "Medium", nullptr));
        soxrResampleQualityComboBox->setItemText(2, QCoreApplication::translate("PreferenceDialog", "High Quality", nullptr));
        soxrResampleQualityComboBox->setItemText(3, QCoreApplication::translate("PreferenceDialog", "Very High Quality", nullptr));
        soxrResampleQualityComboBox->setItemText(4, QCoreApplication::translate("PreferenceDialog", "Ultra High Quality", nullptr));

        groupBox->setTitle(QCoreApplication::translate("PreferenceDialog", "Roll Off:", nullptr));
        lbRollOffLevel->setText(QCoreApplication::translate("PreferenceDialog", "Level:", nullptr));
        rollOffLevelComboBox->setItemText(0, QCoreApplication::translate("PreferenceDialog", "Small (< 0.01 dB)", nullptr));
        rollOffLevelComboBox->setItemText(1, QCoreApplication::translate("PreferenceDialog", "Medium (< 0.35 dB)", nullptr));
        rollOffLevelComboBox->setItemText(2, QCoreApplication::translate("PreferenceDialog", "None", nullptr));

        groupBox_2->setTitle(QCoreApplication::translate("PreferenceDialog", "Passsband:", nullptr));
        soxrPassbandValue->setText(QCoreApplication::translate("PreferenceDialog", "90%", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("PreferenceDialog", "Phase:", nullptr));
        soxrPhaseValue->setText(QCoreApplication::translate("PreferenceDialog", "45%", nullptr));
        lbSrcTargetSampleRate->setText(QCoreApplication::translate("PreferenceDialog", "Target samplerate:", nullptr));
        srcTargetSampleRateComboBox->setItemText(0, QCoreApplication::translate("PreferenceDialog", "44100", nullptr));
        srcTargetSampleRateComboBox->setItemText(1, QCoreApplication::translate("PreferenceDialog", "48000", nullptr));
        srcTargetSampleRateComboBox->setItemText(2, QCoreApplication::translate("PreferenceDialog", "88200", nullptr));
        srcTargetSampleRateComboBox->setItemText(3, QCoreApplication::translate("PreferenceDialog", "96000", nullptr));
        srcTargetSampleRateComboBox->setItemText(4, QCoreApplication::translate("PreferenceDialog", "176400", nullptr));
        srcTargetSampleRateComboBox->setItemText(5, QCoreApplication::translate("PreferenceDialog", "192000", nullptr));
        srcTargetSampleRateComboBox->setItemText(6, QCoreApplication::translate("PreferenceDialog", "352800", nullptr));
        srcTargetSampleRateComboBox->setItemText(7, QCoreApplication::translate("PreferenceDialog", "384000", nullptr));
        srcTargetSampleRateComboBox->setItemText(8, QCoreApplication::translate("PreferenceDialog", "768000", nullptr));

        lbSrcHz->setText(QCoreApplication::translate("PreferenceDialog", "Hz", nullptr));
        lbR8BrainTargetSampleRate->setText(QCoreApplication::translate("PreferenceDialog", "Target samplerate:", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(0, QCoreApplication::translate("PreferenceDialog", "44100", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(1, QCoreApplication::translate("PreferenceDialog", "48000", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(2, QCoreApplication::translate("PreferenceDialog", "88200", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(3, QCoreApplication::translate("PreferenceDialog", "96000", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(4, QCoreApplication::translate("PreferenceDialog", "176400", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(5, QCoreApplication::translate("PreferenceDialog", "192000", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(6, QCoreApplication::translate("PreferenceDialog", "352800", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(7, QCoreApplication::translate("PreferenceDialog", "384000", nullptr));
        r8brainTargetSampleRateComboBox->setItemText(8, QCoreApplication::translate("PreferenceDialog", "768000", nullptr));

        lbR8BrainHz->setText(QCoreApplication::translate("PreferenceDialog", "Hz", nullptr));
        lbResampler->setText(QCoreApplication::translate("PreferenceDialog", "Resampler:", nullptr));
        selectResamplerComboBox->setItemText(0, QCoreApplication::translate("PreferenceDialog", "Disable", nullptr));
        selectResamplerComboBox->setItemText(1, QCoreApplication::translate("PreferenceDialog", "Soxr", nullptr));
        selectResamplerComboBox->setItemText(2, QCoreApplication::translate("PreferenceDialog", "Secret Rabbit Code", nullptr));
        selectResamplerComboBox->setItemText(3, QCoreApplication::translate("PreferenceDialog", "R8brain Free", nullptr));

        resetAllButton->setText(QCoreApplication::translate("PreferenceDialog", "Reset All", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PreferenceDialog: public Ui_PreferenceDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PREFERENCEDIALOG_H
