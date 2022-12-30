//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QTimer>

#include <version.h>

#include <base/exception.h>
#include <widget/xdialog.h>

class QIcon;
class QAbstractButton;
class QGridLayout;
class QLabel;

using xamp::base::Exception;

class MaskWidget : public QWidget {
public:
    explicit MaskWidget(QWidget* parent = nullptr);

    void showEvent(QShowEvent* event) override;
};

class XMessageBox : public XDialog {
public:
    static constexpr int kDefaultTimeoutSecond = 8;

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

    QAbstractButton* defaultButton();

    QDialogButtonBox::StandardButton standardButton(QAbstractButton* button) const;

    QPushButton* addButton(QDialogButtonBox::StandardButton buttons);

    void addWidget(QWidget* widget);

    void showEvent(QShowEvent* event) override;

    static void showBug(const Exception& exception,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showYesOrNo(const QString& text,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr);

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

public slots:
    void updateTimeout();

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

    int timeout_{ kDefaultTimeoutSecond };
    QString defaultButtonText_;
    QLabel* icon_label_;
    QLabel* message_text_label_;
    QGridLayout* grid_layout_;
    QDialogButtonBox* button_box_;
    QAbstractButton* clicked_button_;
    QAbstractButton* default_button_;
    QTimer timer_;
};