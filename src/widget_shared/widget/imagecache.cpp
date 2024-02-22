#include <widget/imagecache.h>

#include <thememanager.h>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/util/image_utiltis.h>
#include <widget/util/str_utilts.h>
#include <widget/qetag.h>
#include <widget/widget_shared.h>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>
#include <base/object_pool.h>

#include <QStringList>
#include <QPixmap>
#include <QBuffer>
#include <QImageWriter>
#include <QDirIterator>
#include <QImageReader>

inline constexpr size_t kDefaultCacheSize = 24;
inline constexpr qint64 kMaxCacheImageSize = 1024 * 1024;
inline auto kCacheFileExtension = qTEXT(".") + qSTR(ImageCache::kImageFileFormat).toLower();

namespace {
	QString makeImageCachePath(const QString& tag_id) {
		return qAppSettings.cachePath() + tag_id + kCacheFileExtension;
	}
}

XAMP_DECLARE_LOG_NAME(ImageCache);

ImageCache::ImageCache()
	: logger_(XampLoggerFactory.GetLogger(kImageCacheLoggerName))
	, cache_(kMaxCacheImageSize) {
	unknown_cover_id_ = qTEXT("unknown_album");
	cache_ext_ =
		QStringList() << (qTEXT("*") + kCacheFileExtension);
	cover_ext_ =
		QStringList() << qTEXT("*.jpeg") << qTEXT("*.jpg") << qTEXT("*.png");
	trim_target_size_ = kMaxCacheImageSize * 3 / 4;
	buffer_pool_ = std::make_shared<ObjectPool<QBuffer>>(kDefaultCacheSize);
	cache_.Add(unknown_cover_id_, { 1, qTheme.unknownCover() });
	loadCache();
	startTimer(kTrimImageSizeSeconds);	
}

QPixmap ImageCache::scanCoverFromDir(const QString& file_path) {
    const QList<QString> target_folders = { qTEXT("scans"), qTEXT("artwork"), qTEXT("booklet") };
    const QString cover_name = qTEXT("Front");

	const QDir dir(file_path);
	QDir scan_dir(dir);

	auto find_image = [scan_dir, this](QDirIterator::IteratorFlags dir_iter_flag) -> std::optional<QPixmap> {
		const QFileInfo file_info(scan_dir.absolutePath());
		const auto path = file_info.absolutePath();
		for (QDirIterator itr(path, cover_ext_, QDir::Files | QDir::NoDotAndDotDot, dir_iter_flag);
			itr.hasNext();) {
			const auto image_file_path = itr.next();
			return image_utils::readFileImage(image_file_path, 
				qTheme.cacheCoverSize(), 
				kFormat);
		}
		return std::nullopt;
	};

	if (auto image = find_image(QDirIterator::NoIteratorFlags)) {
		return image.value();
	}
	/*if (auto image = find_image(QDirIterator::Subdirectories)) {
		return image.value();
	}*/

	// Find 'Scans' folder in the same level or in parent folders.
	while (!scan_dir.isRoot()) {
        if (std::any_of(target_folders.begin(), target_folders.end(), [&](const QString& folder) {
			return scan_dir.entryList(QDir::Dirs).contains(folder, Qt::CaseInsensitive);
			})) {
			// Enter the first found target folder
			scan_dir.cd(target_folders.first());
			break;
		}
		// Parent path maybe contains image file.
		const auto image_files = scan_dir.entryList(cover_ext_, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
		if (!image_files.isEmpty()) {			
			break;
		}
		scan_dir.cdUp();
	}

	// Scan image file in 'Front' file name.
	const auto image_files = scan_dir.entryList(cover_ext_, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
	for (const auto& image_file_path : image_files) {
		if (!image_file_path.toLower().contains(cover_name.toLower())) {
			continue;
		}

		return image_utils::readFileImage(scan_dir.filePath(image_file_path),
			qTheme.cacheCoverSize(),
			kFormat);
	}

	// If no file matches the 'Front' file name, return the first file that matches the name filters.
	if (!image_files.empty()) {
		return image_utils::readFileImage(scan_dir.filePath(image_files[0]),
			qTheme.cacheCoverSize(),
			kFormat);
	}

	return {};
}

void ImageCache::clearCache() const {
	cache_.Clear();
}

void ImageCache::clear() const {
	for (QDirIterator itr(qAppSettings.cachePath(), QDir::Files);
		itr.hasNext();) {
		const auto path = itr.next();
		QFile file(path);
		file.remove();
	}
	cache_.Clear();
}

QPixmap ImageCache::findImageFromDir(const PlayListEntity& item) {
	return scanCoverFromDir(item.file_path);
}

void ImageCache::removeImage(const QString& tag_id) const {
	QFile file(makeImageCachePath(tag_id));
	file.remove();
	cache_.Erase(tag_id);
}

ImageCacheEntity ImageCache::getFromFile(const QString& tag_id) const {
	QImage image(qTheme.cacheCoverSize(), kFormat);
	QImageReader reader(makeImageCachePath(tag_id));
	if (reader.read(&image)) {
		const auto file_info = getImageFileInfo(tag_id);
		return { file_info.size(), QPixmap::fromImage(image) };
	}
	return {};
}

QFileInfo ImageCache::getImageFileInfo(const QString& tag_id) const {
	return QFileInfo(makeImageCachePath(tag_id));
}

QPixmap ImageCache::cover(const QString& tag, const QString& cover_id) {	
	XAMP_LOG_T(logger_, "tag:{} cache-size: {}, cover cache: {}, cache: {}", 
		tag.toStdString(),
		String::FormatBytes(cache_.GetSize()), cover_cache_, cache_);

	return cover_cache_.GetOrAdd(tag + cover_id, [this, tag, cover_id]() {
		return getOrAdd(tag + cover_id, [cover_id, this]() {
			return image_utils::roundImage(
				image_utils::resizeImage(getOrDefault(cover_id), qTheme.defaultCoverSize(), true),
				image_utils::kSmallImageRadius);
			});
		});
}

QPixmap ImageCache::getOrAdd(const QString& tag_id, std::function<QPixmap()>&& value_factory) const {
	auto image = getOrDefault(tag_id, false);
	if (!image.isNull()) {
		return image;
	}

	const auto buffer = buffer_pool_->Acquire();
	if (!buffer->open(QIODevice::WriteOnly)) {
		XAMP_LOG_DEBUG("Failure to create buffer.");
	}

	const auto cache_cover = value_factory();
	const auto file_path = makeImageCachePath(tag_id);
	if (!cache_cover.save(buffer.get(), kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save buffer.");
	}

	if (!cache_cover.save(file_path, kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save image cache.");
	}

	cache_.AddOrUpdate(tag_id, { buffer->size(), cache_cover });
	buffer->close();
	buffer->setData(QByteArray());	
	return getOrDefault(tag_id);
}

QString ImageCache::addImage(const QPixmap& cover) const {
	const auto cover_size = qTheme.cacheCoverSize();

	const auto buffer = buffer_pool_->Acquire();
	if (!buffer->open(QIODevice::WriteOnly)) {
		XAMP_LOG_DEBUG("Failure to create buffer.");
	}

	auto resize_image = image_utils::resizeImage(cover, cover_size, true);
	if (!resize_image.save(buffer.get(), kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save buffer.");
	}

	auto tag_id = qetag::getTagId(buffer->buffer());
	const auto file_path = makeImageCachePath(tag_id);

	if (!resize_image.save(file_path, kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save image cache.");
	}
	
	cache_.AddOrUpdate(tag_id, { buffer->size(), resize_image });
	buffer->close();
	buffer->setData(QByteArray());	
	return tag_id;
}

void ImageCache::loadCache() const {
	Stopwatch sw;

	size_t i = 0;
	for (QDirIterator itr(qAppSettings.cachePath(), cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext(); ++i) {
		const auto path = itr.next();
		QImage image(qTheme.cacheCoverSize(), kFormat);
		QImageReader reader(path);
		if (reader.read(&image)) {
			const QFileInfo file_info(path);
			const auto tag_name = file_info.baseName();
			if (cache_.IsFull(file_info.size())) {
				break;
			}
			cache_.AddOrUpdate(tag_name, { file_info.size(), QPixmap::fromImage(image) });
			XAMP_LOG_D(logger_, "Add tag:{} {} size: {}", tag_name.toStdString(),
				cache_, String::FormatBytes(file_info.size()));
		}
	}

	XAMP_LOG_D(logger_, "Cache count: {} files, elapsed: {}secs, size: {}",
		i,
		sw.ElapsedSeconds(), 
		String::FormatBytes(size()));
}

QPixmap ImageCache::getOrDefault(const QString& tag_id, bool not_found_use_default) const {
	const auto [size, image] = cache_.GetOrAdd(tag_id, [tag_id, this]() {
		XAMP_LOG_D(logger_, "Load tag:{}", tag_id.toStdString());
		return getFromFile(tag_id);
	});

	if (!tag_id.isEmpty()) {
		XAMP_LOG_D(logger_, "Find tag:{} {}", tag_id.toStdString(), cache_);
	}

	if (image.isNull() && not_found_use_default) {
		return qTheme.defaultSizeUnknownCover();
	}
	return image;
}

void ImageCache::setMaxSize(const size_t max_size) {
	trim_target_size_ = max_size;
}

size_t ImageCache::size() const {
	return cache_.GetSize();
}

void ImageCache::timerEvent(QTimerEvent* ) {
	if (cache_.GetSize() > trim_target_size_) {
		cache_.Evict(trim_target_size_);
	}
	XAMP_LOG_D(logger_, "Trim target-cache-size: {}, cover cache: {}, cache: {}", 
		String::FormatBytes(trim_target_size_), cover_cache_, cache_);
}
