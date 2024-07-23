/********************************************************************************
** Form generated from reading UI file 'filesystemviewpage.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILESYSTEMVIEWPAGE_H
#define UI_FILESYSTEMVIEWPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FileSystemViewPage
{
public:
    QHBoxLayout *horizontalLayout;
    QSplitter *splitter;
    QTreeView *dirTree;

    void setupUi(QWidget *FileSystemViewPage)
    {
        if (FileSystemViewPage->objectName().isEmpty())
            FileSystemViewPage->setObjectName("FileSystemViewPage");
        FileSystemViewPage->resize(713, 492);
        horizontalLayout = new QHBoxLayout(FileSystemViewPage);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        splitter = new QSplitter(FileSystemViewPage);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Horizontal);
        splitter->setHandleWidth(3);
        dirTree = new QTreeView(splitter);
        dirTree->setObjectName("dirTree");
        splitter->addWidget(dirTree);

        horizontalLayout->addWidget(splitter);


        retranslateUi(FileSystemViewPage);

        QMetaObject::connectSlotsByName(FileSystemViewPage);
    } // setupUi

    void retranslateUi(QWidget *FileSystemViewPage)
    {
        FileSystemViewPage->setWindowTitle(QCoreApplication::translate("FileSystemViewPage", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FileSystemViewPage: public Ui_FileSystemViewPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILESYSTEMVIEWPAGE_H
