//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialogButtonBox>
#include <QTimer>

#include <version.h>

#include <widget/widget_shared_global.h>
#include <widget/xdialog.h>

class QIcon;
class QAbstractButton;
class QGridLayout;
class QLabel;

using xamp::base::Exception;

namespace xamp::base {
    class Exception;
}

class XAMP_WIDGET_SHARED_EXPORT XMessageBox : public XDialog {
public:
    static constexpr auto kDefaultTimeoutSecond = 8;

	explicit XMessageBox(const QString& title= kEmptyString,
	                     const QString& text = kEmptyString,
	                     QWidget* parent = nullptr,
						 QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
						 QDialogButtonBox::StandardButton default_button = QDialogButtonBox::StandardButton::Ok,
						 bool enable_countdown = false);

    void setDefaultButton(QPushButton* button);

    void setDefaultButton(QDialogButtonBox::StandardButton button);

    void setIcon(const QIcon &icon);

    void setText(const QString& text);

    void setTextFont(const QFont& font);

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
        bool enable_countdown = true,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showInformation(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showWarning(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showError(const QString& text,
        const QString& title = kApplicationTitle,
        bool enable_countdown = false,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static std::tuple<QDialogButtonBox::StandardButton, bool> showCheckBoxQuestion(const QString& text,
        const QString& check_box_text = kEmptyString,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static std::tuple<QDialogButtonBox::StandardButton, bool> showCheckBoxInformation(const QString& text,
        const QString& check_box_text = kEmptyString,
        const QString& title = kApplicationTitle,
        bool enable_countdown = true,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

public slots:
    void onUpdate();

private:
    static std::tuple<QDialogButtonBox::StandardButton, bool> showCheckBox(const QString& text,
        const QString& check_box_text,
        const QString& title,
        bool enable_countdown,
        const QIcon& icon,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    static QDialogButtonBox::StandardButton showButton(const QString& text,
        const QString& title,
        bool enable_countdown,
        const QIcon& icon,
        QFlags<QDialogButtonBox::StandardButton> buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok,
        QWidget* parent = nullptr);

    void onButtonClicked(QAbstractButton* button);

    int execReturnCode(QAbstractButton* button);

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