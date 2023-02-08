#include <widget/pixmapcache.h>

#include "thememanager.h"
#include "appsettings.h"
#include "appsettingnames.h"

#include <widget/image_utiltis.h>
#include <widget/str_utilts.h>
#include <widget/qetag.h>
#include <widget/widget_shared.h>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

#include <QStringList>
#include <QPixmap>
#include <QBuffer>
#include <QImageWriter>
#include <QDirIterator>
#include <QImageReader>

inline constexpr size_t kDefaultCacheSize = 24;
inline constexpr qint64 kMaxCacheImageSize = 10 * 1024 * 1024;
inline auto kCacheFileExtension = qTEXT(".") + qSTR(PixmapCache::kImageFileFormat).toLower();

XAMP_DECLARE_LOG_NAME(PixmapCache);

QStringList PixmapCache::cover_ext_ =
    QStringList() << qTEXT("*.jpeg") << qTEXT("*.jpg") << qTEXT("*.png") << qTEXT("*.bmp");

QStringList PixmapCache::cache_ext_ =
    QStringList() << (qTEXT("*") + kCacheFileExtension);

PixmapCache::PixmapCache()
	: logger_(LoggerManager::GetInstance().GetLogger(kPixmapCacheLoggerName))
	, cache_(kMaxCacheImageSize) {
	unknown_cover_id_ = qTEXT("unknown_album");
	cache_.Add(unknown_cover_id_, qTheme.UnknownCover());
	InitCachePath();
	LoadCache();
	startTimer(10000);
	trim_target_size_ = kMaxCacheImageSize * 3 / 4;
}

void PixmapCache::InitCachePath() {
	if (!AppSettings::contains(kAppSettingAlbumImageCachePath)) {
		const List<QString> paths{
			AppSettings::DefaultCachePath() + qTEXT("/caches/"),
			QDir::currentPath() + qTEXT("/caches/")
		};

		Q_FOREACH(const auto path, paths) {
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
	else {
		cache_path_ = AppSettings::ValueAsString(kAppSettingAlbumImageCachePath);
	}
	cache_path_ = ToNativeSeparators(cache_path_);
	AppSettings::SetValue(kAppSettingAlbumImageCachePath, cache_path_);
}

QPixmap PixmapCache::FindCoverInDir(const QString& file_path) {
	const auto dir = QFileInfo(file_path).path();

    for (QDirIterator itr(dir, cover_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto image_file_path = itr.next();
		QImage image(qTheme.GetCacheCoverSize(), QImage::Format_RGB32);
		QImageReader reader(image_file_path);
		if (reader.read(&image)) {
			return QPixmap::fromImage(image);
		}
	}
	return {};
}

void PixmapCache::ClearCache() {
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

QPixmap PixmapCache::FindCoverInDir(const PlayListEntity& item) {
	return FindCoverInDir(item.file_path);
}

void PixmapCache::erase(const QString& tag_id) {
	QFile file(cache_path_ + tag_id + kCacheFileExtension);
	file.remove();
	cache_.Erase(tag_id);
}

QPixmap PixmapCache::FromFileCache(const QString& tag_id) const {
	QImage image(qTheme.GetCacheCoverSize(), QImage::Format_RGB32);
	QImageReader reader(cache_path_ + tag_id + kCacheFileExtension);
	if (reader.read(&image)) {
		return QPixmap::fromImage(image);
	}
	return {};
}

QFileInfo PixmapCache::CacheFileInfo(const QString& tag_id) const {
	return QFileInfo(cache_path_ + tag_id + kCacheFileExtension);
}

QString PixmapCache::SavePixamp(const QPixmap &cover) {
    QByteArray array;
    QBuffer buffer(&array);

    const auto cover_size = qTheme.GetCacheCoverSize();
    const auto cache_cover = image_utils::ResizeImage(cover, cover_size, true);

	QString tag_name;
	if (cache_cover.save(&buffer, kImageFileFormat)) {
		tag_name = QEtag::GetTagId(array);
		const auto file_path = cache_path_ + tag_name + kCacheFileExtension;
		emit ProcessImage(file_path, array, tag_name);
	}
	return tag_name;
}

void PixmapCache::OptimizeImageFromBuffer(const QString& file_path, const QByteArray& buffer, const QString& tag_name) {
	image_utils::OptimizePng(buffer, file_path);
	cache_.AddOrUpdate(tag_name, FromFileCache(tag_name));
}

void PixmapCache::OptimizeImage(const QString& src_file_path, const QString& dest_file_path, const QString& tag_name) {
	image_utils::OptimizePng(src_file_path, dest_file_path);
	cache_.AddOrUpdate(tag_name, FromFileCache(tag_name));
}

void PixmapCache::OptimizeImageInDir(const QString& file_path) {
	const auto dir = QFileInfo(file_path).path();

	for (QDirIterator itr(dir, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto path = itr.next();
		const QFileInfo image_file_path(path);
		QImage image(qTheme.GetCacheCoverSize(), QImage::Format_RGB32);
		QImageReader reader(path);
		if (reader.read(&image)) {
			auto temp = QPixmap::fromImage(image);
			auto temp_file_name = dir + qTEXT("/") + image_file_path.baseName() + qTEXT("_temp") + kCacheFileExtension;
			auto save_file_name = dir + qTEXT("/") + image_file_path.baseName() + kCacheFileExtension;
			if (temp.save(temp_file_name, kImageFileFormat, 100)) {
				image_utils::OptimizePng(temp_file_name, save_file_name);
				Fs::remove(temp_file_name.toStdWString());
			}
		}
	}
}

void PixmapCache::LoadCache() const {
	Stopwatch sw;

	size_t i = 0;
	for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext(); ++i) {
		const auto path = itr.next();
		QImage image(qTheme.GetCacheCoverSize(), QImage::Format_RGB32);
		QImageReader reader(path);
		if (reader.read(&image)) {
			const QFileInfo image_file_path(path);
			const auto tag_name = image_file_path.baseName();
			cache_.AddOrUpdate(tag_name, QPixmap::fromImage(image));
			XAMP_LOG_D(logger_, "Add tag:{} {}", tag_name.toStdString(), cache_);
		}
	}

	XAMP_LOG_D(logger_, "Cache count: {} files, elapsed: {}secs, size: {}",
		i,
		sw.ElapsedSeconds(), 
		String::FormatBytes(CacheSize()));
}

QPixmap PixmapCache::find(const QString& tag_id) const {
	const auto cover = cache_.GetOrAdd(tag_id, [tag_id, this]() {
		XAMP_LOG_D(logger_, "Load tag:{}", tag_id.toStdString());
		return FromFileCache(tag_id);
	});

	if (!tag_id.isEmpty()) {
		XAMP_LOG_D(logger_, "Find tag:{} {}", tag_id.toStdString(), cache_);
	}

	if (cover.isNull()) {
		return qTheme.DefaultSizeUnknownCover();
	}
	return cover;
}

QString PixmapCache::AddOrUpdate(const QByteArray& data) {
	QPixmap cover;
	cover.loadFromData(data);
    return SavePixamp(cover);
}

void PixmapCache::AddCache(const QString& tag_id, const QPixmap& cover) {
	cache_.AddOrUpdate(tag_id, cover);
}

void PixmapCache::SetMaxSize(size_t max_size) {
    cache_.Resize(max_size);
}

size_t PixmapCache::CacheSize() const {
	size_t size = 0;
	for (auto const & [fst, snd] : cache_) {
		size += CacheFileInfo(fst).size();
	}
	return size;
}

void PixmapCache::timerEvent(QTimerEvent* ) {
	if (cache_.GetSize() > trim_target_size_) {
		cache_.Evict(trim_target_size_);
	}
	XAMP_LOG_D(logger_, "Trim target-cache-size: {}, {}", 
		String::FormatBytes(trim_target_size_), cache_);
}
