#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QPixmapCache>
#include <QPixmap>

#include <base/logger.h>
#include <base/scopeguard.h>

#include "thememanager.h"

#include <widget/image_utiltis.h>
#include <widget/str_utilts.h>
#include <widget/filetag.h>
#include <widget/pixmapcache.h>

static constexpr size_t kDefaultCacheSize = 1024;

PixmapCache::PixmapCache()
    : cache_(kDefaultCacheSize) {
    cache_path_ = QDir::currentPath() + Q_UTF8("/caches/");
    cover_ext_ << Q_UTF8("*.jpeg") << Q_UTF8("*.jpg") << Q_UTF8("*.png") << Q_UTF8("*.bmp");
    cache_ext_ << Q_UTF8("*.cache");
    QDir dir;
    (void)dir.mkdir(cache_path_);
	loadCache();
}

QPixmap PixmapCache::findFileDirCover(const QString& file_path) {
    const auto dir = QFileInfo(file_path).path();

    for (QDirIterator itr(dir, PixmapCache::instance().cover_ext_,
                          QDir::Files | QDir::NoDotAndDotDot);
         itr.hasNext();) {
        const auto image_file_path = itr.next();
        QPixmap read_cover(image_file_path);
        if (!read_cover.isNull()) {
            return read_cover;
        }
    }
    return QPixmap();
}

void PixmapCache::clear() {
	for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto path = itr.next();
		const QFileInfo image_file_path(path);
		QFile file(path);
		file.remove();
		file.close();
	}

	cache_.Clear();
}

QPixmap PixmapCache::findFileDirCover(const PlayListEntity& item) {
    return findFileDirCover(item.file_path);
}

void PixmapCache::erase(const QString& tag_id) {
	QFile file(cache_path_ + tag_id + Q_UTF8(".cache"));
	file.remove();
	cache_.Erase(tag_id);
}

QPixmap PixmapCache::fromFileCache(const QString& tag_id) const {
    return QPixmap(cache_path_ + tag_id + Q_UTF8(".cache"));
}

void PixmapCache::loadCache() const {
    size_t i = 0;
    for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext(); ++i) {
        const auto path = itr.next();

        const QFileInfo image_file_path(path);
		if (i >= cache_.GetMaxSize()) {
			QFile file(path);
			file.remove();
			file.close();
			XAMP_LOG_DEBUG("Remove image cache: {}", image_file_path.baseName().toStdString());
		}
		
        QPixmap read_cover(path);
        if (!read_cover.isNull()) {
            const auto tag_id = image_file_path.baseName();
            cache_.Insert(tag_id, read_cover);
        }
    }

	XAMP_LOG_DEBUG("PixmapCache cache count: {}", i);
}

std::optional<const QPixmap*> PixmapCache::find(const QString& tag_id) const {
	XAMP_ON_SCOPE_EXIT(
		XAMP_LOG_DEBUG("PixmapCache miss {}%", cache_.GetMissCount() * 100 / cache_.GetMaxSize());
		);

	while (true) {
        const auto cache = cache_.Find(tag_id);
		if (!cache) {
			QPixmap read_cover = fromFileCache(tag_id);
			if (read_cover.isNull()) {
				return std::nullopt;
			}
            cache_.Insert(tag_id, read_cover);
			continue;
		}
		return cache;
	}
}

QString PixmapCache::add(const QPixmap& cover) const {
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::WriteOnly);

	auto cover_size = ThemeManager::instance().getCacheCoverSize();

	// Cover必須要忽略圖片比例, 不然從cache抓出來的時候無法正確縮放.
	auto cache_cover = Pixmap::resizeImage(cover, cover_size, true);

	QString tag_name;
	if (cache_cover.save(&buffer, "JPG")) {
		tag_name = FileTag::getTagId(array);
		(void)cache_cover.save(cache_path_ + tag_name + Q_UTF8(".cache"), "JPG", 100);
	}

    cache_.Insert(tag_name, cache_cover);
	return tag_name;
}

bool PixmapCache::isExist(const QString& tag_id) const {
    return cache_.Find(tag_id) != nullptr;
}

size_t PixmapCache::getImageSize() const {
	size_t size = 0;
	for (auto cache : cache_) {
		size += cache.second.size().width()
			* cache.second.size().height()
			* cache.second.devicePixelRatio();
	}
	return size;
}
