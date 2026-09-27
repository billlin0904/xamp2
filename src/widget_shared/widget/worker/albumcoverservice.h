//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QImage>
#include <QObject>

#include <widget/database.h>
#include <widget/widget_shared_global.h>
#include <widget/databasecoverid.h>

class XAMP_WIDGET_SHARED_API AlbumCoverService : public QObject {
	Q_OBJECT
public:
	static constexpr size_t kBufferPoolSize = 256;

	AlbumCoverService();

	void cleanup();	

signals:
	void albumCoverLoaded(int32_t album_id, const QImage& image, bool save_only);

public slots:
	void enableFetchThumbnail(bool enable);

	void onFindAlbumCover(const DatabaseCoverId& id);

	void removeAlbumCoverId(int32_t album_id);

	void clearAlbumCoverIds();

	void cancelRequested();

private:
	bool is_stop_{ false };	
	bool enable_{ true };
	PooledDatabasePtr database_ptr_;
	HashSet<int32_t> pending_album_cover_ids_;
	HashSet<int32_t> completed_album_cover_ids_;
	LoggerPtr logger_;
};

