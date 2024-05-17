//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

#include <deque>

#include <widget/database.h>
#include <widget/widget_shared_global.h>
#include <widget/databasecoverid.h>

class XAMP_WIDGET_SHARED_EXPORT FindAlbumCoverWorker : public QObject {
	Q_OBJECT
public:
	static constexpr size_t kBufferPoolSize = 256;

	FindAlbumCoverWorker();

signals:
	void fetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url);

	void setThumbnail(const DatabaseCoverId &id, const QString& cover_id);

	void setAlbumCover(int32_t album_id, const QString& cover_id);

public slots:
	void onFindAlbumCover(const DatabaseCoverId& id);

	void onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url);

	void cancelRequested();

	void onLookupAlbumCover(const DatabaseCoverId& id, const Path& path);

private slots:
	void onFetchAlbumCover();

private:
	void fetchAlbumCover(const DatabaseCoverId& id, const Path& file_path);

	bool is_stop_{ false };	
	PooledDatabasePtr database_ptr_;
	QNetworkAccessManager nam_;
	QTimer timer_;
	std::deque<std::pair<DatabaseCoverId, Path>> fetch_album_cover_queue_;
	std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
};

