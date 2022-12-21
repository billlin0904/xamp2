//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QDialogButtonBox>
#include <QMessageBox>

#include <version.h>
#include <widget/xdialog.h>

class QIcon;
class QAbstractButton;
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
						 QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
						 QDialogButtonBox::StandardButton default_button = QDialogButtonBox::StandardButton::Ok);

    void setDefaultButton(QPushButton* button);

    void setDefaultButton(QDialogButtonBox::StandardButton button);

    void setIcon(const QIcon &icon);

    void setText(const QString& text);

    QAbstractButton* clickedButton() const;

    QDialogButtonBox::StandardButton standardButton(QAbstractButton* button) const;

    QPushButton* addButton(QDialogButtonBox::StandardButton buttons);

    void addWidget(QWidget* widget);

    static QDialogButtonBox::StandardButton showInformation(const QString& text,
        const QString& title = kApplicationTitle,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showWarning(const QString& text,
        const QString& title = kApplicationTitle,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showError(const QString& text,
        const QString& title = kApplicationTitle,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showCheckBoxQuestion(const QString& text,
        const QString& check_box_text = qEmptyString,
        const QString& title = kApplicationTitle,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showCheckBoxInformation(const QString& text,
        const QString& check_box_text = qEmptyString,
        const QString& title = kApplicationTitle,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

private:
    static QDialogButtonBox::StandardButton showCheckBox(const QString& text,
        const QString& check_box_text,
        const QString& title,
        const QIcon& icon,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showButton(const QString& text,
        const QString& title,
        const QIcon& icon,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    void onButtonClicked(QAbstractButton* button);

    int execReturnCode(QAbstractButton* button);

    QLabel* icon_label_;
    QLabel* message_text_label_;
    QGridLayout* grid_layout_;
    QDialogButtonBox* button_box_;
    QAbstractButton* clicked_button_;
    QAbstractButton* default_button_;
};