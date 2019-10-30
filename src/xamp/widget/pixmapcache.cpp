#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QPixmapCache>
#include <QPixmap>

#include <base/logger.h>

#include "str_utilts.h"
#include "filetag.h"
#include "pixmapcache.h"

constexpr int32_t MAX_CACHE_SIZE = 1024;

PixmapCache::PixmapCache()
    : cache_(MAX_CACHE_SIZE) {
    cache_path_ = QDir::currentPath() + Q_UTF8("/caches/");
    cover_ext_ << Q_UTF8("*.jpeg") << Q_UTF8("*.jpg") << Q_UTF8("*.png") << Q_UTF8("*.bmp");
    cache_ext_ << Q_UTF8("*.cache");
    QDir dir;
    (void)dir.mkdir(cache_path_);
    loadCache();
}

QPixmap PixmapCache::findDirExistCover(const QString& file_path) {
    const auto dir = QFileInfo(file_path).path();

    QPixmap read_cover;

    for (QDirIterator itr(dir, PixmapCache::Instance().cover_ext_, QDir::Files | QDir::NoDotAndDotDot); itr.hasNext();) {
        const auto image_file_path = itr.next();
        read_cover = QPixmap(image_file_path);
        if (!read_cover.isNull()) {
            break;
        }
    }
    return read_cover;
}

QPixmap PixmapCache::findDirExistCover(const PlayListEntity& item) {
	return findDirExistCover(item.file_path);
}

void PixmapCache::erase(const QString& tag_id) {
	QFile file(cache_path_ + tag_id + Q_UTF8(".cache"));
	file.remove();
	cache_.erase(tag_id);
}

void PixmapCache::loadCache() const {
    for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);  itr.hasNext(); ) {
        const auto path = itr.next();

        const QFileInfo image_file_path(path);
        QPixmap read_cover(path);
        if (!read_cover.isNull()) {
            const auto tag_id = image_file_path.baseName();
			cache_.insert(tag_id, std::move(read_cover));
        }
    }
}

const QPixmap* PixmapCache::find(const QString& tag_id) const {
	while (true) {
		const auto cache = cache_.findOrNull(tag_id);
		if (!cache) {
			QPixmap read_cover(cache_path_ + tag_id + Q_UTF8(".cache"));
			if (read_cover.isNull()) {
				return nullptr;
			}
			cache_.insert(tag_id, std::move(read_cover));
			continue;
		}
		return cache;
	}
}

QString PixmapCache::emplace(QPixmap&& cover) const {
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::WriteOnly);

	QString tag_name;
	if (cover.save(&buffer, "JPG")) {
		tag_name = FileTag::getTagId(array);
		(void)cover.save(cache_path_ + tag_name + Q_UTF8(".cache"), "JPG", 100);
	}

	cache_.emplace(tag_name, std::move(cover));
	return tag_name;
}

bool PixmapCache::isExist(const QString& tag_id) const {
    return cache_.findOrNull(tag_id) != nullptr;
}
