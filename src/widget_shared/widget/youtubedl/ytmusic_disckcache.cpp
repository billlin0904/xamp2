#include <QFile>
#include <QDir>
#include <QDirIterator>

#include <widget/appsettings.h>
#include <widget/util/str_util.h>
#include <widget/youtubedl/ytmusic_disckcache.h>

inline constexpr qint64 kMaxDiskCacheSize = 64 * 1024 * 1024;

YtMusicDiskCache::YtMusicDiskCache()
	: cache_(kMaxDiskCacheSize) {
}

QString YtMusicDiskCache::makeFileCachePath(const QString& video_id) {
	auto cache_path = qAppSettings.getOrCreateCachePath() + "DiskCache/"_str;
	auto file_path = cache_path + video_id + ".mp4"_str;
	const QDir dir(cache_path);
	if (!dir.exists()) {
		if (!dir.mkdir(cache_path)) {
		}
	}
	return file_path;
}

void YtMusicDiskCache::load() {
	auto cache_ext = QStringList() << "*"_str + ".mp4"_str;

	for (QDirIterator itr(qAppSettings.getOrCreateCachePath() + "DiskCache/"_str, cache_ext, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto path = itr.next();
		QFileInfo file_info(path);
		auto video_id = file_info.baseName();
		setFileName(video_id, path);
	}
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