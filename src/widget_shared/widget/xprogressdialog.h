//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/xdialog.h>
#include <widget/util/str_utilts.h>
#include <widget/widget_shared_global.h>

class QProgressBar;
class QAbstractButton;
class QVBoxLayout;

class XAMP_WIDGET_SHARED_EXPORT XProgressDialog : public XDialog {
	Q_OBJECT
public:
	explicit XProgressDialog(const QString& title= kEmptyString,
		const QString& cancel_text = kEmptyString,
		int minimum = 0, 
		int maximum = 100,
		QWidget* parent = nullptr);

	void setRange(int minimum, int maximum);

	void setValue(int value);

	int value() const;

	void setLabelText(const QString& text);

	void setSubValue(int total, int current);

	bool wasCanceled() const;

signals:
	void cancelRequested();

private:
	int max_width_ = 800;
	QAbstractButton* default_button_;
	QProgressBar* progress_bar_;
	QLabel* message_text_label_;
	QLabel* sub_text_label_;
	QVBoxLayout* layout_;
	QSize size_;
};
