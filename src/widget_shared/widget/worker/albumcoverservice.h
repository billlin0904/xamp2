//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

#include <set>

#include <widget/database.h>
#include <widget/widget_shared_global.h>
#include <widget/databasecoverid.h>
#include <widget/httpx.h>

class XAMP_WIDGET_SHARED_EXPORT AlbumCoverService : public QObject {
	Q_OBJECT
public:
	static constexpr size_t kBufferPoolSize = 256;

	AlbumCoverService();

	void mergeUnknownAlbumCover();

	void cleaup();	

signals:
	void fetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url);

	void setThumbnail(const DatabaseCoverId &id, const QString& cover_id);

	void setAristThumbnail(int32_t artist_id, const QString& cover_id);

	void setAlbumCover(int32_t album_id, const QString& cover_id);

public slots:
	void enableFetchThumbnail(bool enable);

	void onFindAlbumCover(const DatabaseCoverId& id);

	void onFetchYoutubeThumbnailUrl(const QString& video_id, const QString& thumbnail_url);

	void onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url);

	void onFetchArtistThumbnailUrl(int32_t artist_id, const QString& thumbnail_url);

	void cancelRequested();

private:
	bool is_stop_{ false };	
	bool enable_{ true };
	PooledDatabasePtr database_ptr_;
	QNetworkAccessManager nam_;
	http::HttpClient http_client_;
	std::set<QString> pending_requests_;
};

