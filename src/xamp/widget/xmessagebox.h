//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QMessageBox>

#include <version.h>
#include <widget/xdialog.h>

class QIcon;
class QAbstractButton;
class QDialogButtonBox;
class QGridLayout;
class QLabel;

class MaskWidget : public QWidget {
public:
    explicit MaskWidget(QWidget* parent = nullptr);

    void showEvent(QShowEvent* event) override;
};

class XMessageBox : public XDialog {
public:
	explicit XMessageBox(const QString& title = qEmptyString,
	                     const QString& text = qEmptyString,
	                     QWidget* parent = nullptr,
	                     QMessageBox::StandardButton buttons = QMessageBox::StandardButton::Ok,
	                     QMessageBox::StandardButton default_button = QMessageBox::StandardButton::Ok);

    void setDefaultButton(QPushButton* button);

    void setDefaultButton(QMessageBox::StandardButton button);

    void setIcon(const QIcon &icon);

    void setText(const QString& text);

    QAbstractButton* clickedButton() const;

    QMessageBox::StandardButton standardButton(QAbstractButton* button) const;

    void addButton(QMessageBox::StandardButton buttons);

    void addWidget(QWidget* widget);

    static QMessageBox::StandardButton showInformation(const QString& text,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr,
        QMessageBox::StandardButton buttons = QMessageBox::StandardButton::Ok);

    static QMessageBox::StandardButton showWarning(const QString& text,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr,
        QMessageBox::StandardButton buttons = QMessageBox::StandardButton::Ok);

    static QMessageBox::StandardButton showError(const QString& text,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr,
        QMessageBox::StandardButton buttons = QMessageBox::StandardButton::Ok);

    static QMessageBox::StandardButton showCheckBoxQuestion(const QString& text,
        const QString& check_box_text = qEmptyString,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr,
        QMessageBox::StandardButton buttons = QMessageBox::StandardButton::Ok,
        QMessageBox::StandardButton default_button = QMessageBox::StandardButton::Ok);

private:
    static QMessageBox::StandardButton showButton(const QString& text,
        const QString& title,
        const QIcon& icon,
        QWidget* parent = nullptr,
        QMessageBox::StandardButton buttons = QMessageBox::StandardButton::Ok);

    void onButtonClicked(QAbstractButton* button);

    int execReturnCode(QAbstractButton* button);

    QLabel* iconLabel_;
    QLabel* messageTextLabel_;
    QGridLayout* gridLayout_;
    QDialogButtonBox* buttonBox_;
    QAbstractButton* clickedButton_;
    QAbstractButton* defaultButton_;
};