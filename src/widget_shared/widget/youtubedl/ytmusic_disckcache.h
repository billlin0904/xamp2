//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>
#include <QObject>

#include <base/lrucache.h>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT FileCacheEntity {
	FileCacheEntity(int64_t size, const QString& video_id, const QString &file_name)
		: size(size)
		, video_id(video_id)
		, file_name(file_name) {
	}
	int64_t size;
	QString video_id;
	QString file_name;
};

struct FileCacheSizeOfPolicy {
	int64_t operator()(const QString&, const FileCacheEntity& entity) const noexcept {
		return entity.size;
	}
};

class XAMP_WIDGET_SHARED_EXPORT YtMusicDiskCache {
public:
	YtMusicDiskCache();

	static QString makeFileCachePath(const QString& video_id);

	bool isCached(const QString & video_id) const;

	FileCacheEntity getFileName(const QString& video_id) const;

	void setFileName(const QString& video_id, const QString& file_name);
private:
	mutable LruCache<QString, FileCacheEntity, FileCacheSizeOfPolicy> cache_;
};

#define qDiskCache SharedSingleton<YtMusicDiskCache>::GetInstance()
