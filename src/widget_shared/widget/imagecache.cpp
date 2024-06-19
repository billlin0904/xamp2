#include <widget/imagecache.h>

#include <thememanager.h>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/util/image_util.h>
#include <widget/util/str_util.h>
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

constexpr size_t kDefaultCacheSize = 24;
constexpr qint64 kMaxCacheImageSize = 32 * 1024 * 1024;
auto kCacheFileExtension = qTEXT(".") + qSTR(ImageCache::kImageFileFormat).toLower();

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
	loadUnknownCover();
	loadCache();
	startTimer(kTrimImageSizeSeconds);	
}

void ImageCache::onThemeChangedFinished(ThemeColor theme_color) {
	loadUnknownCover();
}

void ImageCache::loadUnknownCover() {
	auto unknown_cover = qTheme.unknownCover();
	cache_.Add(unknown_cover_id_, { 1, unknown_cover });
	const auto file_path = makeImageCachePath(kAlbumCacheTag + unknown_cover_id_);
	unknown_cover.save(file_path);
	addOrUpdateCover(kAlbumCacheTag, unknown_cover_id_, unknown_cover);
}

QPixmap ImageCache::scanCoverFromDir(const QString& file_path) {
    const QList<QString> kTargetFolders = { qTEXT("scans"), qTEXT("artwork"), qTEXT("booklet") };    
	constexpr auto kMaxDirCdUp = 4;

	// 1...
	// 2..
	// 3.Disc1
	// 4.Disc2
	// 5.Disc3
	// 6.Disc4
	// 7.Disc5
	// 8.Disc6
	// 9.Disc7
	// 10.Scans
	constexpr auto kMaxUnexceptedDirSize = 10;

	const QDir dir(file_path);
	QDir scan_dir(dir);

	auto find_dir_image = [this](const QDir &scan_dir, QDirIterator::IteratorFlags dir_iter_flag) -> std::optional<QPixmap> {
		const QString kFrontCoverName = qTEXT("Front");

		QStringList image_file_list;
		for (QDirIterator itr(scan_dir.path(), cover_ext_, QDir::Files | QDir::NoDotAndDotDot, dir_iter_flag);
			itr.hasNext();) {
			const auto image_file_path = itr.next();
			image_file_list.append(image_file_path);
		}

		if (image_file_list.isEmpty()) {
			return std::nullopt;
		}

		std::sort(image_file_list.begin(), image_file_list.end(), [](const auto& a, const auto& b) {
			auto file_index_a = QFileInfo(a).baseName().toStdWString();
			auto file_index_b = QFileInfo(b).baseName().toStdWString();
			auto index_a = 0;
			port_swscanf(file_index_a.c_str(), L"%d", &index_a);
			auto index_b = 0;
			port_swscanf(file_index_b.c_str(), L"%d", &index_b);
			return index_a < index_b;
			});

		auto find_cover_path = image_file_list[0];

		for (const auto& image_file_path : image_file_list) {
			if (image_file_path.contains(kFrontCoverName, Qt::CaseInsensitive)) {
				find_cover_path = image_file_path;
				break;
			}			
		}

		return std::optional<QPixmap> {
			std::in_place_t{},
				image_util::readFileImage(find_cover_path,
					qTheme.cacheCoverSize(),
					kImageFormat) };
	};

	// 1. Scan image file in the same level.
	if (auto image = find_dir_image(QDir(dir.absolutePath()), QDirIterator::NoIteratorFlags)) {
		return image.value();
	}

	// 2. Find 'Scans' folder in the same level or in parent folders.
	auto cd_up_count = 0;
	while (!scan_dir.isRoot() && cd_up_count < kMaxDirCdUp) {
		bool found = false;
		auto dirs = scan_dir.entryList(QDir::Dirs);
		if (dirs.count() > kMaxUnexceptedDirSize) {
			return {};
		}
		for (const auto& folder : kTargetFolders) {			
			for (const auto& dir : dirs) {
				if (dir.contains(folder, Qt::CaseInsensitive)) {
					scan_dir.cd(dir);
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
		}
		auto now_path = scan_dir.path();
		// Parent path maybe contains image file.
		if (auto image = find_dir_image(scan_dir, QDirIterator::Subdirectories)) {
			return image.value();
		}
		scan_dir.cdUp();
		now_path = scan_dir.path();
		++cd_up_count;
	}

	return {};
}

void ImageCache::clearCache() const {
	cache_.Clear();
}

QString ImageCache::makeCacheFilePath() const {
	return qAppSettings.getOrCreateCachePath() + qTEXT("ImageCache/");
}

QString ImageCache::makeImageCachePath(const QString& tag_id) const {
	return makeCacheFilePath() + tag_id + kCacheFileExtension;
}

void ImageCache::clear() const {
	for (QDirIterator itr(makeCacheFilePath(), cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto path = itr.next();
		QFile file(path);
		if (!file.remove()) {
			XAMP_LOG_D(logger_, "Failure to remove cache file: {}", path.toStdString());
		}
	}
	cache_.Clear();
}

QPixmap ImageCache::findImageFromDir(const PlayListEntity& item) {
	return scanCoverFromDir(item.file_path);
}

void ImageCache::removeImage(const QString& tag_id) const {
	auto path = makeImageCachePath(tag_id);
	QFile file(path);
	if (!file.remove()) {
		XAMP_LOG_D(logger_, "Failure to remove cache file: {}", path.toStdString());
	}
	cache_.Erase(tag_id);
}

ImageCacheEntity ImageCache::getFromFile(const QString& tag_id) const {
	if (tag_id.isEmpty()) {
		return {};
	}
	QImage image(qTheme.cacheCoverSize(), kImageFormat);
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

QPixmap ImageCache::getOrDefault(const QString& tag, const QString& cover_id) {	
	XAMP_LOG_T(logger_, "tag:{} cache-size: {}, cover cache: {}, cache: {}", 
		tag.toStdString(),
		String::FormatBytes(cache_.GetSize()), cover_cache_, cache_);

	return getOrAdd(tag + cover_id, [cover_id, this]() {	
		auto is_aspect_ratio = true;
		if (cover_id == unknownCoverId()) {
			is_aspect_ratio = false;
		}
		return image_util::roundImage(
			image_util::resizeImage(getOrAddDefault(cover_id, false), qTheme.defaultCoverSize(), is_aspect_ratio),
			image_util::kSmallImageRadius);
		});
}

void ImageCache::addOrUpdateCover(const QString& tag, const QString& cover_id, const QPixmap& cover) const {
	getOrAdd(tag + cover_id, [&cover, cover_id, this]() {
		auto is_aspect_ratio = true;
		if (cover_id == unknownCoverId()) {
			is_aspect_ratio = false;
		}
		return image_util::roundImage(
			image_util::resizeImage(cover, qTheme.defaultCoverSize(), is_aspect_ratio),
			image_util::kSmallImageRadius);
		});
}

QPixmap ImageCache::getOrAdd(const QString& tag_id, std::function<QPixmap()>&& value_factory) const {
	auto image = getOrAddDefault(tag_id, false);
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
	return getOrAddDefault(tag_id);
}

QString ImageCache::addImage(const QPixmap& cover, bool save_only) const {
	const auto cover_size = qTheme.cacheCoverSize();

	const auto buffer = buffer_pool_->Acquire();
	if (!buffer->open(QIODevice::WriteOnly)) {
		XAMP_LOG_DEBUG("Failure to create buffer.");
	}

	auto resize_image = image_util::resizeImage(cover, cover_size, true);
	if (!resize_image.save(buffer.get(), kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save buffer.");
	}

	auto tag_id = qetag::getTagId(buffer->buffer());
	const auto file_path = makeImageCachePath(tag_id);

	if (!resize_image.save(file_path, kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save image cache.");
	}

	if (save_only) {
		buffer->close();
		buffer->setData(QByteArray());

		return tag_id;
	}
	
	cache_.AddOrUpdate(tag_id, { buffer->size(), resize_image });
	buffer->close();
	buffer->setData(QByteArray());	

	addOrUpdateCover(kAlbumCacheTag, tag_id, resize_image);
	return tag_id;
}

void ImageCache::loadCache() const {
	Stopwatch sw;

	size_t i = 0;
	for (QDirIterator itr(makeCacheFilePath(), cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext(); ++i) {
		const auto path = itr.next();
		QImage image(qTheme.cacheCoverSize(), kImageFormat);
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

QPixmap ImageCache::getOrAddDefault(const QString& tag_id, bool not_found_use_default) const {
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

QIcon ImageCache::uniformIcon(const QIcon& icon, QSize size) const {
	QIcon result;
	const auto base_pixmap = icon.pixmap(size);
	for (const auto state : { QIcon::Off, QIcon::On }) {
		for (const auto mode : { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected })
			result.addPixmap(base_pixmap, mode, state);
	}
	return result;
}

QIcon ImageCache::getOrAddIcon(const QString& id) const {
	return qIconCache.GetOrAdd(id, [id, this]() {
		const QIcon icon(image_util::roundImage(qImageCache.getOrAddDefault(id), kCoverSize));
		return uniformIcon(icon, kCoverSize);
		});
}

void ImageCache::addOrUpdateIcon(const QString& id, const QIcon& value) const {
	qIconCache.AddOrUpdate(id, value);
}

void ImageCache::timerEvent(QTimerEvent* ) {
	if (cache_.GetSize() > trim_target_size_) {
		cache_.Evict(trim_target_size_);
	}
	XAMP_LOG_T(logger_, "Trim target-cache-size: {}, cover cache: {}, cache: {}", 
		String::FormatBytes(trim_target_size_), cover_cache_, cache_);
}
