//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>

namespace Ui {
	class PlaybackQueueViewPage;
}

class XAMP_WIDGET_SHARED_EXPORT PlaybackQueueViewPage : public QDialog {
	Q_OBJECT
public:
	explicit PlaybackQueueViewPage(QWidget* parent = nullptr);

	~PlaybackQueueViewPage() override;

	QString popQueue();

	void addQueue(const TrackInfo &track_info);

	void clearQueue();

signals:
	void playFile(const QString& file_name, bool queue);

private:
	void paintEvent(QPaintEvent* event) override;
	Ui::PlaybackQueueViewPage* ui_;
};