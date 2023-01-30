//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/xdialog.h>
#include <widget/str_utilts.h>

class QProgressBar;
class QAbstractButton;
class QVBoxLayout;

class XProgressDialog : public XDialog {
public:
	explicit XProgressDialog(const QString& title= qEmptyString,
		const QString& cancel_text = qEmptyString,
		int minimum = 0, 
		int maximum = 100,
		QWidget* parent = nullptr);

	void SetRange(int minimum, int maximum);

	void SetValue(int value);

	void SetLabelText(const QString& text);

	bool WasCanceled() const;
private:
	QAbstractButton* default_button_;
	QProgressBar* progress_bar_;
	QLabel* message_text_label_;
	QVBoxLayout* layout_;
	QSize size_;
};
