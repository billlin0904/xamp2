//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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
    static constexpr auto kDefaultTimeoutSecond = 8;

	explicit XMessageBox(const QString& title= qEmptyString,
	                     const QString& text = qEmptyString,
	                     QWidget* parent = nullptr,
						 QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
						 QDialogButtonBox::StandardButton default_button = QDialogButtonBox::StandardButton::Ok,
						 bool enable_countdown = false);

    void SetDefaultButton(QPushButton* button);

    void SetDefaultButton(QDialogButtonBox::StandardButton button);

    void SetIcon(const QIcon &icon);

    void SetText(const QString& text);

    void SetTextFont(const QFont& font);

    QAbstractButton* ClickedButton() const;

    QAbstractButton* DefaultButton();

    QDialogButtonBox::StandardButton StandardButton(QAbstractButton* button) const;

    QPushButton* AddButton(QDialogButtonBox::StandardButton buttons);

    void AddWidget(QWidget* widget);

    void showEvent(QShowEvent* event) override;

    static void ShowBug(const Exception& exception,
        const QString& title = kApplicationTitle,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton ShowYesOrNo(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton ShowInformation(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton ShowWarning(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton ShowError(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = false,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static std::tuple<QDialogButtonBox::StandardButton, bool> ShowCheckBoxQuestion(const QString& text,
        const QString& check_box_text = qEmptyString,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static std::tuple<QDialogButtonBox::StandardButton, bool> ShowCheckBoxInformation(const QString& text,
        const QString& check_box_text = qEmptyString,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

public slots:
    void UpdateTimeout();

private:
    static std::tuple<QDialogButtonBox::StandardButton, bool> ShowCheckBox(const QString& text,
        const QString& check_box_text,
        const QString& title,
        bool enable_countdown,
        const QIcon& icon,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton ShowButton(const QString& text,
        const QString& title,
        bool enable_countdown,
        const QIcon& icon,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    void OnButtonClicked(QAbstractButton* button);

    int ExecReturnCode(QAbstractButton* button);

    bool enable_countdown_;
    int timeout_{ kDefaultTimeoutSecond };
    QString default_button_text_;
    QLabel* icon_label_;
    QLabel* message_text_label_;
    QGridLayout* grid_layout_;
    QDialogButtonBox* button_box_;
    QAbstractButton* clicked_button_;
    QAbstractButton* default_button_;
    QTimer timer_;
};