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

inline constexpr size_t kDefaultCacheSize = 32;

PixmapCache::PixmapCache()
	: cache_(kDefaultCacheSize) {
	cache_path_ = QDir::currentPath() + Q_UTF8("/caches/");
	cover_ext_ << Q_UTF8("*.jpeg") << Q_UTF8("*.jpg") << Q_UTF8("*.png") << Q_UTF8("*.bmp");
	cache_ext_ << Q_UTF8("*.cache");
	const QDir dir;
	(void)dir.mkdir(cache_path_);
	unknown_cover_id_ = addOrUpdate(QPixmap(Q_UTF8(":/xamp/Resource/White/unknown_album.png")));
}

QPixmap PixmapCache::findFileDirCover(const QString& file_path) {
	const auto dir = QFileInfo(file_path).path();

	for (QDirIterator itr(dir, Singleton<PixmapCache>::GetInstance().cover_ext_,
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
			cache_.AddOrUpdate(tag_id, read_cover);
		}
	}

	XAMP_LOG_DEBUG("PixmapCache cache count: {}", i);
}

const QPixmap* PixmapCache::find(const QString& tag_id) const {
	while (true) {
		const auto* const cache = cache_.Find(tag_id);
		if (!cache) {
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
	QByteArray array;
	QBuffer buffer(&array);
	QPixmap cover;
	cover.loadFromData(data);

	const auto cover_size = ThemeManager::instance().getCacheCoverSize();
	const auto cache_cover = Pixmap::resizeImage(cover, cover_size, true);

	QString tag_name;
	if (cache_cover.save(&buffer, "JPG")) {
		tag_name = FileTag::getTagId(array);
		(void)cache_cover.save(cache_path_ + tag_name + Q_UTF8(".cache"), "JPG", 100);
	}
	return tag_name;
}

QString PixmapCache::addOrUpdate(const QPixmap& cover) const {
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::WriteOnly);
	const auto cover_size = ThemeManager::instance().getCacheCoverSize();

	// Cover必須要忽略圖片比例, 不然從cache抓出來的時候無法正確縮放.
	const auto cache_cover = Pixmap::resizeImage(cover, cover_size, true);

	QString tag_name;
	if (cache_cover.save(&buffer, "JPG")) {
		tag_name = FileTag::getTagId(array);
		(void)cache_cover.save(cache_path_ + tag_name + Q_UTF8(".cache"), "JPG", 100);
	}
	return tag_name;
}

bool PixmapCache::isExist(const QString& tag_id) const {
	return cache_.Find(tag_id) != nullptr;
}

size_t PixmapCache::getImageSize() const {
	size_t size = 0;
	for (auto const & [fst, snd] : cache_) {
		size += snd.size().width()
			* snd.size().height()
			* snd.devicePixelRatio();
	}
	return size;
}
