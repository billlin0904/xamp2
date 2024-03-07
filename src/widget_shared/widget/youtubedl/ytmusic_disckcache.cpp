#include <QFile>
#include <QDir>
#include <widget/appsettings.h>
#include <widget/util/str_utilts.h>
#include <widget/youtubedl/ytmusic_disckcache.h>

YtMusicDiskCache::YtMusicDiskCache()
	: cache_(64 * 1024 * 1024) {
}

QString YtMusicDiskCache::makeFileCachePath(const QString& video_id) {
	auto cache_path = qAppSettings.cachePath() + qTEXT("DiskCache/");
	auto file_path = cache_path + video_id + qTEXT(".mp4");
	const QDir dir(cache_path);
	if (!dir.exists()) {
		if (!dir.mkdir(cache_path)) {
		}
	}
	return file_path;
}

bool YtMusicDiskCache::isCached(const QString& video_id) const {
	return cache_.Contains(video_id);
}

FileCacheEntity YtMusicDiskCache::getFileName(const QString& video_id) const {
	return cache_.GetOrAdd(video_id, [this, video_id]() {
		QFile file(makeFileCachePath(video_id));
		if (!file.exists()) {
			return FileCacheEntity{ 0, kEmptyString, kEmptyString };
		}
		return FileCacheEntity { file.size(), video_id, file.fileName()};
		});
}

void YtMusicDiskCache::setFileName(const QString& video_id, const QString& file_name) {
	QFile file(file_name);
	if (!file.exists()) {
		return;
	}	
	cache_.AddOrUpdate(video_id, FileCacheEntity{ file.size(), video_id, file_name });
}