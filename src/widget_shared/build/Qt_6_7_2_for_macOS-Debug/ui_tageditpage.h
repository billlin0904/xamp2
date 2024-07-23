/********************************************************************************
** Form generated from reading UI file 'tageditpage.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAGEDITPAGE_H
#define UI_TAGEDITPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_TagEditPage
{
public:
    QGridLayout *gridLayout;
    QLineEdit *albumReplayGainLineEdit;
    QLabel *trackReplayGainLabel;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QLabel *notFoundImageLabel;
    QLabel *coverLabel;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_3;
    QLabel *coverSizeLabel;
    QSpacerItem *horizontalSpacer_4;
    QPushButton *addImageFileButton;
    QPushButton *removeCoverButton;
    QPushButton *saveToFileButton;
    QLineEdit *fileSizeLineEdit;
    QLabel *artistLabel;
    QLabel *albumReplayGainLabel;
    QLineEdit *artistLineEdit;
    QLabel *trackLabel;
    QLabel *audioMd5Label;
    QLineEdit *albumPeakLineEdit;
    QLabel *audioMd5Label_2;
    QLineEdit *commentLineEdit;
    QLabel *titleLabel;
    QLabel *commentLabel;
    QLabel *albumPeakLabel;
    QLineEdit *albumLineEdit;
    QLineEdit *yearLineEdit;
    QLabel *filePathLabel;
    QLabel *albumLabel;
    QComboBox *titleComboBox;
    QComboBox *trackComboBox;
    QLineEdit *trackPeakLineEdit;
    QDialogButtonBox *buttonBox;
    QComboBox *genreComboBox;
    QLabel *yearLabel;
    QLabel *trackPeakLabel;
    QSpacerItem *verticalSpacer;
    QLabel *genreLabel;
    QLineEdit *trackReplayGainLineEdit;
    QPushButton *clearCommentButton;
    QLineEdit *audioMD5LineEdit;
    QFrame *frame;
    QLineEdit *filePathLineEdit;

    void setupUi(QFrame *TagEditPage)
    {
        if (TagEditPage->objectName().isEmpty())
            TagEditPage->setObjectName("TagEditPage");
        TagEditPage->resize(746, 745);
        gridLayout = new QGridLayout(TagEditPage);
        gridLayout->setObjectName("gridLayout");
        albumReplayGainLineEdit = new QLineEdit(TagEditPage);
        albumReplayGainLineEdit->setObjectName("albumReplayGainLineEdit");
        albumReplayGainLineEdit->setReadOnly(true);

        gridLayout->addWidget(albumReplayGainLineEdit, 10, 5, 1, 1);

        trackReplayGainLabel = new QLabel(TagEditPage);
        trackReplayGainLabel->setObjectName("trackReplayGainLabel");

        gridLayout->addWidget(trackReplayGainLabel, 11, 3, 1, 1);

        groupBox = new QGroupBox(TagEditPage);
        groupBox->setObjectName("groupBox");
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        notFoundImageLabel = new QLabel(groupBox);
        notFoundImageLabel->setObjectName("notFoundImageLabel");
        notFoundImageLabel->setMinimumSize(QSize(250, 250));
        notFoundImageLabel->setMaximumSize(QSize(250, 250));
        notFoundImageLabel->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(notFoundImageLabel);

        coverLabel = new QLabel(groupBox);
        coverLabel->setObjectName("coverLabel");
        coverLabel->setMinimumSize(QSize(250, 250));
        coverLabel->setMaximumSize(QSize(250, 250));
        coverLabel->setFrameShape(QFrame::StyledPanel);

        horizontalLayout_2->addWidget(coverLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        coverSizeLabel = new QLabel(groupBox);
        coverSizeLabel->setObjectName("coverSizeLabel");
        coverSizeLabel->setMinimumSize(QSize(0, 30));
        coverSizeLabel->setMaximumSize(QSize(16777215, 30));

        horizontalLayout->addWidget(coverSizeLabel);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);

        addImageFileButton = new QPushButton(groupBox);
        addImageFileButton->setObjectName("addImageFileButton");
        addImageFileButton->setMaximumSize(QSize(100, 16777215));

        horizontalLayout->addWidget(addImageFileButton);

        removeCoverButton = new QPushButton(groupBox);
        removeCoverButton->setObjectName("removeCoverButton");

        horizontalLayout->addWidget(removeCoverButton);

        saveToFileButton = new QPushButton(groupBox);
        saveToFileButton->setObjectName("saveToFileButton");

        horizontalLayout->addWidget(saveToFileButton);


        verticalLayout->addLayout(horizontalLayout);


        gridLayout->addWidget(groupBox, 0, 2, 1, 4);

        fileSizeLineEdit = new QLineEdit(TagEditPage);
        fileSizeLineEdit->setObjectName("fileSizeLineEdit");
        fileSizeLineEdit->setReadOnly(true);

        gridLayout->addWidget(fileSizeLineEdit, 14, 2, 1, 2);

        artistLabel = new QLabel(TagEditPage);
        artistLabel->setObjectName("artistLabel");

        gridLayout->addWidget(artistLabel, 4, 0, 1, 1);

        albumReplayGainLabel = new QLabel(TagEditPage);
        albumReplayGainLabel->setObjectName("albumReplayGainLabel");

        gridLayout->addWidget(albumReplayGainLabel, 10, 3, 1, 1);

        artistLineEdit = new QLineEdit(TagEditPage);
        artistLineEdit->setObjectName("artistLineEdit");

        gridLayout->addWidget(artistLineEdit, 4, 2, 1, 4);

        trackLabel = new QLabel(TagEditPage);
        trackLabel->setObjectName("trackLabel");

        gridLayout->addWidget(trackLabel, 8, 0, 1, 2);

        audioMd5Label = new QLabel(TagEditPage);
        audioMd5Label->setObjectName("audioMd5Label");

        gridLayout->addWidget(audioMd5Label, 13, 0, 1, 1);

        albumPeakLineEdit = new QLineEdit(TagEditPage);
        albumPeakLineEdit->setObjectName("albumPeakLineEdit");
        albumPeakLineEdit->setReadOnly(true);

        gridLayout->addWidget(albumPeakLineEdit, 10, 2, 1, 1);

        audioMd5Label_2 = new QLabel(TagEditPage);
        audioMd5Label_2->setObjectName("audioMd5Label_2");

        gridLayout->addWidget(audioMd5Label_2, 14, 0, 1, 1);

        commentLineEdit = new QLineEdit(TagEditPage);
        commentLineEdit->setObjectName("commentLineEdit");

        gridLayout->addWidget(commentLineEdit, 6, 2, 1, 3);

        titleLabel = new QLabel(TagEditPage);
        titleLabel->setObjectName("titleLabel");

        gridLayout->addWidget(titleLabel, 1, 0, 1, 1);

        commentLabel = new QLabel(TagEditPage);
        commentLabel->setObjectName("commentLabel");

        gridLayout->addWidget(commentLabel, 6, 0, 1, 2);

        albumPeakLabel = new QLabel(TagEditPage);
        albumPeakLabel->setObjectName("albumPeakLabel");

        gridLayout->addWidget(albumPeakLabel, 10, 0, 1, 1);

        albumLineEdit = new QLineEdit(TagEditPage);
        albumLineEdit->setObjectName("albumLineEdit");

        gridLayout->addWidget(albumLineEdit, 5, 2, 1, 4);

        yearLineEdit = new QLineEdit(TagEditPage);
        yearLineEdit->setObjectName("yearLineEdit");

        gridLayout->addWidget(yearLineEdit, 8, 5, 1, 1);

        filePathLabel = new QLabel(TagEditPage);
        filePathLabel->setObjectName("filePathLabel");

        gridLayout->addWidget(filePathLabel, 3, 0, 1, 1);

        albumLabel = new QLabel(TagEditPage);
        albumLabel->setObjectName("albumLabel");

        gridLayout->addWidget(albumLabel, 5, 0, 1, 1);

        titleComboBox = new QComboBox(TagEditPage);
        titleComboBox->setObjectName("titleComboBox");
        titleComboBox->setEditable(true);

        gridLayout->addWidget(titleComboBox, 1, 2, 1, 4);

        trackComboBox = new QComboBox(TagEditPage);
        trackComboBox->setObjectName("trackComboBox");
        trackComboBox->setEditable(false);

        gridLayout->addWidget(trackComboBox, 8, 2, 1, 1);

        trackPeakLineEdit = new QLineEdit(TagEditPage);
        trackPeakLineEdit->setObjectName("trackPeakLineEdit");
        trackPeakLineEdit->setReadOnly(true);

        gridLayout->addWidget(trackPeakLineEdit, 11, 2, 1, 1);

        buttonBox = new QDialogButtonBox(TagEditPage);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 17, 5, 1, 1);

        genreComboBox = new QComboBox(TagEditPage);
        genreComboBox->setObjectName("genreComboBox");

        gridLayout->addWidget(genreComboBox, 7, 2, 1, 1);

        yearLabel = new QLabel(TagEditPage);
        yearLabel->setObjectName("yearLabel");

        gridLayout->addWidget(yearLabel, 8, 3, 1, 1);

        trackPeakLabel = new QLabel(TagEditPage);
        trackPeakLabel->setObjectName("trackPeakLabel");

        gridLayout->addWidget(trackPeakLabel, 11, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        gridLayout->addItem(verticalSpacer, 15, 0, 1, 1);

        genreLabel = new QLabel(TagEditPage);
        genreLabel->setObjectName("genreLabel");

        gridLayout->addWidget(genreLabel, 7, 0, 1, 1);

        trackReplayGainLineEdit = new QLineEdit(TagEditPage);
        trackReplayGainLineEdit->setObjectName("trackReplayGainLineEdit");
        trackReplayGainLineEdit->setReadOnly(true);

        gridLayout->addWidget(trackReplayGainLineEdit, 11, 5, 1, 1);

        clearCommentButton = new QPushButton(TagEditPage);
        clearCommentButton->setObjectName("clearCommentButton");
        clearCommentButton->setMaximumSize(QSize(100, 16777215));

        gridLayout->addWidget(clearCommentButton, 6, 5, 1, 1);

        audioMD5LineEdit = new QLineEdit(TagEditPage);
        audioMD5LineEdit->setObjectName("audioMD5LineEdit");
        audioMD5LineEdit->setReadOnly(true);

        gridLayout->addWidget(audioMD5LineEdit, 13, 2, 1, 4);

        frame = new QFrame(TagEditPage);
        frame->setObjectName("frame");
        frame->setFrameShape(QFrame::HLine);
        frame->setFrameShadow(QFrame::Raised);

        gridLayout->addWidget(frame, 16, 0, 1, 6);

        filePathLineEdit = new QLineEdit(TagEditPage);
        filePathLineEdit->setObjectName("filePathLineEdit");
        filePathLineEdit->setReadOnly(true);

        gridLayout->addWidget(filePathLineEdit, 3, 2, 1, 4);


        retranslateUi(TagEditPage);

        QMetaObject::connectSlotsByName(TagEditPage);
    } // setupUi

    void retranslateUi(QFrame *TagEditPage)
    {
        TagEditPage->setWindowTitle(QCoreApplication::translate("TagEditPage", "Frame", nullptr));
        trackReplayGainLabel->setText(QCoreApplication::translate("TagEditPage", "Track ReplayGain:", nullptr));
        groupBox->setTitle(QCoreApplication::translate("TagEditPage", "Embedded Cover", nullptr));
        notFoundImageLabel->setText(QCoreApplication::translate("TagEditPage", "Not found image", nullptr));
        coverLabel->setText(QString());
        coverSizeLabel->setText(QCoreApplication::translate("TagEditPage", "None", nullptr));
        addImageFileButton->setText(QCoreApplication::translate("TagEditPage", "Clear", nullptr));
        removeCoverButton->setText(QCoreApplication::translate("TagEditPage", "Remove", nullptr));
        saveToFileButton->setText(QCoreApplication::translate("TagEditPage", "Save to File", nullptr));
        artistLabel->setText(QCoreApplication::translate("TagEditPage", "Artist:", nullptr));
        albumReplayGainLabel->setText(QCoreApplication::translate("TagEditPage", "Album ReplayGain:", nullptr));
        trackLabel->setText(QCoreApplication::translate("TagEditPage", "Track:", nullptr));
        audioMd5Label->setText(QCoreApplication::translate("TagEditPage", "Audio MD5:", nullptr));
        audioMd5Label_2->setText(QCoreApplication::translate("TagEditPage", "File Size:", nullptr));
        titleLabel->setText(QCoreApplication::translate("TagEditPage", "Title:", nullptr));
        commentLabel->setText(QCoreApplication::translate("TagEditPage", "Comment:", nullptr));
        albumPeakLabel->setText(QCoreApplication::translate("TagEditPage", "Album Peak:", nullptr));
        filePathLabel->setText(QCoreApplication::translate("TagEditPage", "File Path:", nullptr));
        albumLabel->setText(QCoreApplication::translate("TagEditPage", "Album:", nullptr));
        yearLabel->setText(QCoreApplication::translate("TagEditPage", "Year:", nullptr));
        trackPeakLabel->setText(QCoreApplication::translate("TagEditPage", "Track Peak:", nullptr));
        genreLabel->setText(QCoreApplication::translate("TagEditPage", "Genre:", nullptr));
        clearCommentButton->setText(QCoreApplication::translate("TagEditPage", "Clear", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TagEditPage: public Ui_TagEditPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAGEDITPAGE_H
