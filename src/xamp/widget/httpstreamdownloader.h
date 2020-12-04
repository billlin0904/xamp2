//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QUrl>
#include <QList>
#include <QTimer>

class HttpStreamDownloader : public QObject {
	Q_OBJECT
public:
	explicit HttpStreamDownloader(QObject *parent = nullptr);	

	void addQueue(int32_t music_id, const QUrl &url, const QString &file_name);

signals:
	void downloadComplete(int32_t music_id, const QString &file_name);

	void stateChange(int32_t music_id);

private:
	void download();
	
	struct DownloadEntry {
		int32_t music_id;		
		const QUrl url;
		const QString file_name;
	};

	QTimer timer_;
	QList<DownloadEntry> entries_;
};

