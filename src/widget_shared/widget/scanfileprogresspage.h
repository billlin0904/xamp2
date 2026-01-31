//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/widget_shared_global.h>

class QProgressBar;
class QLabel;
class QPushButton;

class XAMP_WIDGET_SHARED_EXPORT ScanFileProgressPage : public QFrame {
	Q_OBJECT
public:
	explicit ScanFileProgressPage(QWidget* parent = nullptr);

signals:
	void cancelRequested();

public slots:
	void setOnlyShowProgress();

	void setFileProgress(int32_t progress);

	void onReadCompleted();

	void onRemainingTimeEstimation(size_t total_work, size_t completed_work, int32_t secs);

	void setFileCount(size_t file_count);

	void onReadFilePath(const QString& file_path);

	void onReadFileStart();
private:
	QProgressBar* progress_bar_;
	QPushButton* close_button_;
	QLabel* message_text_label_;	
};
