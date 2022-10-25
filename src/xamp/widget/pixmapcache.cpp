#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QPixmapCache>
#include <QPixmap>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/logger_impl.h>
#include "thememanager.h"
#include "appsettings.h"
#include "appsettingnames.h"

#include <widget/image_utiltis.h>
#include <widget/str_utilts.h>
#include <widget/qetag.h>
#include <widget/pixmapcache.h>

inline constexpr size_t kDefaultCacheSize = 32;
inline constexpr qint64 kMaxCacheImageSize = 8 * 1024 * 1024;
inline constexpr auto kPixmapCacheFileExt = Q_TEXT(".jpg");

QStringList PixmapCache::cover_ext_ =
    QStringList() << Q_TEXT("*.jpeg") << Q_TEXT("*.jpg") << Q_TEXT("*.png") << Q_TEXT("*.bmp");

QStringList PixmapCache::cache_ext_ =
    QStringList() << kPixmapCacheFileExt;

PixmapCache::PixmapCache()
	: cache_(kDefaultCacheSize)
    , logger_(LoggerManager::GetInstance().GetLogger("PixmapCache")) {
	if (!AppSettings::contains(kAppSettingAlbumImageCachePath)) {
		const List<QString> paths{
			AppSettings::defaultCachePath() + Q_TEXT("/caches/"),
			Q_TEXT("/caches/")
		};

		Q_FOREACH(auto path, paths) {
			cache_path_ = path;
			const QDir dir(cache_path_);
			if (!dir.exists()) {
				if (!dir.mkdir(cache_path_)) {
					XAMP_LOG_E(logger_, "Create cache dir faulure!");
				}
				else {					
					break;
				}
			}
		}
	}
	AppSettings::setValue(kAppSettingAlbumImageCachePath, cache_path_);
    unknown_cover_id_ = savePixamp(qTheme.unknownCover());
	loadCache();
}

QPixmap PixmapCache::findFileDirCover(const QString& file_path) {
	const auto dir = QFileInfo(file_path).path();

    for (QDirIterator itr(dir, cover_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto image_file_path = itr.next();
		QFileInfo file_info(image_file_path);
		if (file_info.size() > kMaxCacheImageSize) {
			continue;
		}
		QPixmap read_cover(image_file_path);
		if (!read_cover.isNull()) {
			return read_cover;
		}
	}
	return QPixmap();
}

void PixmapCache::clearCache() {
	cache_.Clear();
}

void PixmapCache::clear() {
	for (QDirIterator itr(cache_path_, QDir::Files);
		itr.hasNext();) {
		const auto path = itr.next();
		QFile file(path);
		file.remove();
	}
	cache_.Clear();
}

QPixmap PixmapCache::findFileDirCover(const PlayListEntity& item) {
	return findFileDirCover(item.file_path);
}

void PixmapCache::erase(const QString& tag_id) {
	QFile file(cache_path_ + tag_id + kPixmapCacheFileExt);
	file.remove();
	cache_.Erase(tag_id);
}

QPixmap PixmapCache::fromFileCache(const QString& tag_id) const {
	return QPixmap(cache_path_ + tag_id + kPixmapCacheFileExt);
}

QString PixmapCache::savePixamp(const QPixmap &cover) const {
    QByteArray array;
    QBuffer buffer(&array);

    const auto cover_size = qTheme.cacheCoverSize();
    const auto cache_cover = Pixmap::scaledImage(cover, cover_size, true);

    QString tag_name;
    if (cache_cover.save(&buffer, "JPG")) {
        tag_name = QEtag::getTagId(array);
        if (!cache_cover.save(cache_path_ + tag_name + kPixmapCacheFileExt, "JPG", 100)) {
            XAMP_LOG_D(logger_, "PixmapCache add file name:{} save error!", tag_name.toStdString());
        } else {
            XAMP_LOG_D(logger_, "PixmapCache add file name:{} save success!", tag_name.toStdString());
        }
    }
    return tag_name;
}

void PixmapCache::loadCache() const {
	size_t i = 0;
	for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext(); ++i) {
		const auto path = itr.next();

		const QFileInfo image_file_path(path);
        if (i >= cache_.GetMaxSize()) {
            XAMP_LOG_D(logger_, "Not load image cache: {}", image_file_path.baseName().toStdString());
            continue;
		}

		QPixmap read_cover(path);
		if (!read_cover.isNull()) {
			const auto tag_name = image_file_path.baseName();
			cache_.AddOrUpdate(tag_name, read_cover);
			XAMP_LOG_D(logger_, "PixmapCache add file name:{}", tag_name.toStdString());
		}
	}

	XAMP_LOG_D(logger_, "PixmapCache cache count: {}", i);
}

size_t PixmapCache::missRate() const {
	if (cache_.GetHitCount() == 0) {
		return 0;
	}
    return cache_.GetMissCount() * 100 / cache_.GetHitCount();
}

const QPixmap* PixmapCache::find(const QString& tag_id) const {
	while (true) {
		const auto* const cache = cache_.Find(tag_id);
		if (!cache) {
            if (tag_id.isEmpty()) {
                return nullptr;
            }
            XAMP_LOG_D(logger_, "PixmapCache load file name:{} from disk, miss: {}",
                       tag_id.toStdString(), missRate());
			auto read_cover = fromFileCache(tag_id);
			if (read_cover.isNull()) {
				return nullptr;
			}
			cache_.AddOrUpdate(tag_id, read_cover);
			continue;
		}
		return cache;
	}
}

QString PixmapCache::addOrUpdate(const QByteArray& data) const {
	QPixmap cover;
	cover.loadFromData(data);
    return savePixamp(cover);
}

void PixmapCache::setMaxSize(size_t max_size) {
    cache_.SetMaxSize(max_size);
}

bool PixmapCache::isExist(const QString& tag_id) const {
	return cache_.Find(tag_id) != nullptr;
}

size_t PixmapCache::totalCacheSize() const {
	size_t size = 0;
	for (auto const & [fst, snd] : cache_) {
		size += snd.size().width()
			* snd.size().height()
			* snd.devicePixelRatio();
	}
	return size;
}
