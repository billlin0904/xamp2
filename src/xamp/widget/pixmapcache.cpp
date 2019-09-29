#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QPixmapCache>
#include <QPixmap>

#include <base/logger.h>

#include "filetag.h"
#include "pixmapcache.h"

PixmapCache::PixmapCache() {
    cache_path_ = QDir::currentPath() + "/caches/";
    cover_ext_ << "*.jpeg" << "*.jpg" << "*.png" << "*.bmp";
    cache_ext_ << "*.cache";
	QPixmapCache::setCacheLimit(10 * 1024);
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
	QPixmapCache::remove(tag_id);
	QFile file(cache_path_ + tag_id + ".cache");
	file.remove();
}

void PixmapCache::loadCache() const {
    for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);  itr.hasNext(); ) {
        const auto path = itr.next();

        const QFileInfo image_file_path(path);
        QPixmap read_cover(path);
        if (!read_cover.isNull()) {
            const auto tag_id = image_file_path.baseName();
			QPixmapCache::insert(tag_id, read_cover);
        }
    }
}

const QPixmap* PixmapCache::find(const QString& tag_id) const {
	while (true) {
		const auto cache = QPixmapCache::find(tag_id);
		if (!cache) {
			QPixmap read_cover(cache_path_ + tag_id + ".cache");
			if (read_cover.isNull()) {
				return nullptr;
			}
			if (!QPixmapCache::insert(tag_id, read_cover)) {
				XAMP_LOG_DEBUG("insert image cache failure! tag id:{}", tag_id.toStdString());
				return nullptr;
			}
			else {
				continue;
			}
		}
		return cache;
	}
}

bool PixmapCache::find(const QString& tag_id, QPixmap& cover) const {
	const auto cache = QPixmapCache::find(tag_id);
	if (!cache) {
		QPixmap read_cover(cache_path_ + tag_id + ".cache");
		if (read_cover.isNull()) {
			return false;
		}
		QPixmapCache::insert(tag_id, read_cover);
		cover = read_cover.copy();
		return true;
	}
	cover = cache->copy();
    return !cover.isNull();
}

bool PixmapCache::insert(const QPixmap &cover, QString *cover_tag_id) {
    auto tag_id = savePixmap(cover);
    if (cover_tag_id != nullptr) {
        *cover_tag_id = tag_id;
    }
	return QPixmapCache::insert(tag_id, cover);
}

bool PixmapCache::exist(const QString& cover_id) const {
	return QPixmapCache::find(cover_id) != nullptr;
}

QString PixmapCache::savePixmap(const QPixmap& pixmap) const {
    QDir dir;
    (void)dir.mkdir(cache_path_);

    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);

    QString tag_name;
    if (pixmap.save(&buffer, "JPG")) {
        tag_name = FileTag::getTagId(array);
        (void)pixmap.save(cache_path_ + tag_name + ".cache", "JPG", 100);
    }

    return tag_name;
}
