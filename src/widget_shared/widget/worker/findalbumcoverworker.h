//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QNetworkAccessManager>
#include <QObject>

#include <widget/database.h>
#include <widget/widget_shared_global.h>
#include <widget/databasecoverid.h>

class XAMP_WIDGET_SHARED_EXPORT FindAlbumCoverWorker : public QObject {
	Q_OBJECT
public:
	FindAlbumCoverWorker();

signals:
	void fetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url);

	void setThumbnail(const DatabaseCoverId &id, const QString& cover_id);

	void setAlbumCover(int32_t album_id, const QString& cover_id);

public slots:
	void onFindAlbumCover(int32_t album_id, const std::wstring& file_path);

	void onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url);

	void cancelRequested();

private:
	bool is_stop_{ false };	
	PooledDatabasePtr database_ptr_;
	QNetworkAccessManager nam_;
	std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
};

