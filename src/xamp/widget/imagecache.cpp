#include <widget/imagecache.h>

#include <thememanager.h>
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
inline constexpr qint64 kMaxCacheImageSize = 3 * 1024 * 1024;
inline auto kCacheFileExtension = qTEXT(".") + qSTR(ImageCache::kImageFileFormat).toLower();

XAMP_DECLARE_LOG_NAME(PixmapCache);

QStringList ImageCache::cover_ext_ =
    QStringList() << qTEXT("*.jpeg") << qTEXT("*.jpg") << qTEXT("*.png");

QStringList ImageCache::cache_ext_ =
    QStringList() << (qTEXT("*") + kCacheFileExtension);

ImageCache::ImageCache()
	: logger_(LoggerManager::GetInstance().GetLogger(kPixmapCacheLoggerName))
	, cache_(kMaxCacheImageSize) {
	unknown_cover_id_ = qTEXT("unknown_album");
	cache_.Add(unknown_cover_id_, { 1, qTheme.UnknownCover() });
	InitCachePath();
	LoadCache();
	startTimer(kTrimImageSizeSeconds);
	trim_target_size_ = kMaxCacheImageSize * 3 / 4;
}

void ImageCache::InitCachePath() {
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

QPixmap ImageCache::ScanImageFromDir(const QString& file_path) {
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

void ImageCache::ClearCache() {
	cache_.Clear();
}

void ImageCache::Clear() {
	for (QDirIterator itr(cache_path_, QDir::Files);
		itr.hasNext();) {
		const auto path = itr.next();
		QFile file(path);
		file.remove();
	}
	cache_.Clear();
}

QPixmap ImageCache::FindImageFromDir(const PlayListEntity& item) {
	return ScanImageFromDir(item.file_path);
}

void ImageCache::RemoveImage(const QString& tag_id) {
	QFile file(cache_path_ + tag_id + kCacheFileExtension);
	file.remove();
	cache_.Erase(tag_id);
}

ImageCacheEntity ImageCache::GetFromFile(const QString& tag_id) const {
	QImage image(qTheme.GetCacheCoverSize(), QImage::Format_RGB32);
	QImageReader reader(cache_path_ + tag_id + kCacheFileExtension);
	if (reader.read(&image)) {
		const auto file_info = GetImageFileInfo(tag_id);
		return { file_info.size(), QPixmap::fromImage(image) };
	}
	return {};
}

QFileInfo ImageCache::GetImageFileInfo(const QString& tag_id) const {
	return QFileInfo(cache_path_ + tag_id + kCacheFileExtension);
}

QString ImageCache::AddImage(const QPixmap& cover) const {
	QByteArray array;
    QBuffer buffer(&array);

	Stopwatch sw;
    const auto cover_size = qTheme.GetCacheCoverSize();

	// Resize PNG image size.
    const auto cache_cover = image_utils::ResizeImage(cover, cover_size, true);
	XAMP_LOG_DEBUG("Resize image {} secs", sw.ElapsedSeconds());

	sw.Reset();
	QString tag_name;

	// Change image file format to PNG format.
	if (cache_cover.save(&buffer, kImageFileFormat)) {
		XAMP_LOG_DEBUG("Save PNG format {} secs", sw.ElapsedSeconds());

		tag_name = QEtag::GetTagId(array);
		const auto file_path = cache_path_ + tag_name + kCacheFileExtension;
		OptimizeImageFromBuffer(file_path, array, tag_name);
	}
	return tag_name;
}

void ImageCache::OptimizeImageFromBuffer(const QString& file_path, const QByteArray& buffer, const QString& tag_name) const {
	// Reduce PNG image file size and save file to disk and cache.
	image_utils::OptimizePng(buffer, file_path);
	cache_.AddOrUpdate(tag_name, GetFromFile(tag_name));
}

void ImageCache::OptimizeImageInDir(const QString& file_path) const {
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

void ImageCache::LoadCache() const {
	Stopwatch sw;

	size_t i = 0;
	for (QDirIterator itr(cache_path_, cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext(); ++i) {
		const auto path = itr.next();
		QImage image(qTheme.GetCacheCoverSize(), QImage::Format_RGB32);
		QImageReader reader(path);
		if (reader.read(&image)) {
			const QFileInfo file_info(path);
			const auto tag_name = file_info.baseName();
			cache_.AddOrUpdate(tag_name, { file_info.size(), QPixmap::fromImage(image) });
			XAMP_LOG_D(logger_, "Add tag:{} {}", tag_name.toStdString(), cache_);
		}
	}

	XAMP_LOG_D(logger_, "Cache count: {} files, elapsed: {}secs, size: {}",
		i,
		sw.ElapsedSeconds(), 
		String::FormatBytes(GetSize()));
}

QPixmap ImageCache::GetOrDefault(const QString& tag_id, bool not_found_use_default) const {
	const auto [size, image] = cache_.GetOrAdd(tag_id, [tag_id, this]() {
		XAMP_LOG_D(logger_, "Load tag:{}", tag_id.toStdString());
		return GetFromFile(tag_id);
	});

	if (!tag_id.isEmpty()) {
		XAMP_LOG_D(logger_, "Find tag:{} {}", tag_id.toStdString(), cache_);
	}

	if (image.isNull() && not_found_use_default) {
		return qTheme.DefaultSizeUnknownCover();
	}
	return image;
}

void ImageCache::SetMaxSize(size_t max_size) {
	trim_target_size_ = max_size;
}

size_t ImageCache::GetSize() const {
	return cache_.GetSize();
}

void ImageCache::timerEvent(QTimerEvent* ) {
	if (cache_.GetSize() > trim_target_size_) {
		cache_.Evict(trim_target_size_);
	}
	XAMP_LOG_D(logger_, "Trim target-cache-size: {}, {}", 
		String::FormatBytes(trim_target_size_), cache_);
}