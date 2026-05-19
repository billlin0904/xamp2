//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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

	void cleanup();	

signals:
	void fetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url);

	void setThumbnail(const DatabaseCoverId &id, const QString& cover_id);

	void setArtistThumbnail(int32_t artist_id, const QString& cover_id);

	void setAlbumCover(int32_t album_id, const QString& cover_id);

public slots:
	void enableFetchThumbnail(bool enable);

	void onFindAlbumCover(const DatabaseCoverId& id);

	void onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url);

	void onFetchArtistThumbnailUrl(int32_t artist_id, const QString& thumbnail_url);

	void onRequestLoad(const QString& tag, const QString& cover_id);

	void cancelRequested();

private:
	void timerEvent(QTimerEvent*) override;

	bool is_stop_{ false };	
	bool enable_{ true };
	PooledDatabasePtr database_ptr_;
	QNetworkAccessManager nam_;
	http::HttpClient http_client_;
	HashSet<QString> pending_request_urls_;
	HashSet<QString> request_load_cover_ids_;
	LoggerPtr logger_;
};

