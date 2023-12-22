//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/str_utilts.h>

class QVBoxLayout;
class QLabel;
class QProgressBar;
class QAbstractButton;

class ProgressView : public QFrame {
	Q_OBJECT
public:
	explicit ProgressView(QWidget* parent = nullptr,
		const QString& cancel_text = kEmptyString,
		int minimum = 0,
		int maximum = 100);

	virtual ~ProgressView() override;

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

