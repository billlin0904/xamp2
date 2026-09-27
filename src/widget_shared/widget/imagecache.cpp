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
#include <base/stopwatch.h>
#include <base/str_utilts.h>
#include <base/object_pool.h>

#include <QStringList>
#include <QPixmap>
#include <QBuffer>
#include <QFile>
#include <QImageWriter>
#include <QDirIterator>
#include <QImageReader>

#include <widget/dao/dbfacade.h>

constexpr size_t kDefaultCacheSize = 24;
constexpr qint64 kMaxCacheImageSize = 8 * 1024 * 1024; //256 * 1024 * 1024;
auto kCacheFileExtension = "."_str + qFormat(ImageCache::kImageFileFormat).toLower();

XAMP_DECLARE_LOG_NAME(ImageCache);

namespace {
	QPixmap makeDisplayCover(const QPixmap& cover) {
		return image_util::roundImage(
			image_util::resizeImage(cover, qTheme.defaultCoverSize(), true),
			image_util::kSmallImageRadius);
	}

	QString makeImageCachePath(const QString& tag_id) {
		return qAppSettings.getOrCreateImageCachePath() + tag_id + kCacheFileExtension;
	}

	QFileInfo getImageFileInfo(const QString& tag_id) {
		return QFileInfo(makeImageCachePath(tag_id));
	}

	bool prepareBuffer(QBuffer& buffer) {
		buffer.close();
		buffer.setData(QByteArray());
		return buffer.open(QIODevice::WriteOnly);
	}

	void resetBuffer(QBuffer& buffer) {
		buffer.close();
		buffer.setData(QByteArray());
	}

	bool writeCacheFile(const QString& file_path, const QByteArray& image_data) {
		QFile file(file_path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			return false;
		}
		return file.write(image_data) == image_data.size();
	}	
}

ImageCache::ImageCache()
	: logger_(XAMP_LOG_CREATE_LOGGER(ImageCache))
	, cache_(kMaxCacheImageSize) {
	unknown_cover_id_ = "unknown_album"_str;
	cache_ext_ =
		QStringList() << "*"_str + kCacheFileExtension;
	cover_ext_ =
		QStringList() << "*.jpeg"_str << "*.jpg"_str << "*.png"_str;
	trim_target_size_ = kMaxCacheImageSize * 3 / 4;
	buffer_pool_ = std::make_shared<ObjectPool<QBuffer>>(kDefaultCacheSize);
	loadUnknownCover();
	loadCache();
	startTimer(kTrimImageSizeSeconds);	
}

void ImageCache::loadUnknownCover() {
	auto unknown_cover = qTheme.unknownCover();	
	for (const auto& tag_id : { unknown_cover_id_, kAlbumCacheTag + unknown_cover_id_ }) {
		const auto file_path = makeImageCachePath(tag_id);
		QFileInfo file_info(file_path);
		if (!file_info.exists()) {
			unknown_cover.save(file_path);
		}
	}
}

void ImageCache::clearCache() const {
	for (QDirIterator itr(qAppSettings.getOrCreateImageCachePath(), cache_ext_, QDir::Files | QDir::NoDotAndDotDot);
		itr.hasNext();) {
		const auto path = itr.next();
		QFile file_(path);
		if (!file_.remove()) {
			XAMP_LOG_D(logger_, "Failure to remove cache file: {}", path.toStdString());
		}
	}
	cache_.clear();
}

void ImageCache::removeCoverId(const QString& cover_id) const {
	if (cover_id.isEmpty() || cover_id == unknownCoverId()) {
		return;
	}

	cache_.erase(cover_id);

	const auto file_path = makeImageCachePath(cover_id);
	QFile file(file_path);
	if (file.exists() && !file.remove()) {
		XAMP_LOG_D(logger_, "Failure to remove image cache file: {}", file_path.toStdString());
	}
}

ImageCacheEntity ImageCache::getFromFile(const QString& cover_id) const {
	if (cover_id.isEmpty()) {
		return {};
	}
	QImage image(qTheme.cacheCoverSize(), kImageFormat);
	QImageReader reader(makeImageCachePath(cover_id));
	if (reader.read(&image)) {
		const auto file_info = getImageFileInfo(cover_id);
		return { file_info.size(), QPixmap::fromImage(image) };
	}
	return {};
}

bool ImageCache::saveCacheImage(const QString& cover_id, const QPixmap& image, bool update_memory, qint64* encoded_size) const {
	if (cover_id.isEmpty() || image.isNull()) {
		return false;
	}
	const auto buffer = buffer_pool_->acquire();
	if (!prepareBuffer(*buffer)) {
		XAMP_LOG_DEBUG("Failure to create buffer.");
		return false;
	}
	XAMP_ON_SCOPE_EXIT(resetBuffer(*buffer););

	if (!image.save(buffer.get(), kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save buffer.");
		return false;
	}
	const auto image_data = buffer->buffer();
	if (encoded_size != nullptr) {
		*encoded_size = image_data.size();
	}

	const auto file_path = makeImageCachePath(cover_id);
	if (!writeCacheFile(file_path, image_data)) {
        XAMP_LOG_DEBUG("Failure to save image cache. ({})", file_path.toStdString());
		return false;
	}

	if (update_memory) {
		cache_.addOrUpdate(cover_id, { image_data.size(), image });
	}
	return true;
}

void ImageCache::addOrUpdateCover(const QString& cover_id, const QPixmap& cover) const {
	if (cover_id.isEmpty() || cover.isNull()) {
		return;
	}
	const auto cache_key = cover_id;
	ImageCacheEntity entity;
	if (cache_.tryGet(cache_key, entity) && !entity.image.isNull()) {
		return;
	}
	(void) saveCacheImage(cache_key, makeDisplayCover(cover), true);
}

QString ImageCache::addImage(const QPixmap& cover, bool save_only) {
	Stopwatch total_elapsed;
	Stopwatch stage_elapsed;
	const auto cover_size = qTheme.cacheCoverSize();

	const auto buffer = buffer_pool_->acquire();
	if (!prepareBuffer(*buffer)) {
		XAMP_LOG_DEBUG("Failure to create buffer.");
	}

	QPixmap resize_image;
	if (cover.size().width() > cover_size.width() || cover.size().height() > cover_size.height()) {
		resize_image = image_util::resizeImage(cover, cover_size, true);
	} else {
		resize_image = cover;
	}
	const auto resize_elapsed = stage_elapsed.elapsedSeconds();
	
	stage_elapsed.reset();
	if (!resize_image.save(buffer.get(), kImageFileFormat)) {
		XAMP_LOG_DEBUG("Failure to save buffer.");
	}
	const auto encode_elapsed = stage_elapsed.elapsedSeconds();

	auto tag_id = qetag::getTagId(buffer->buffer());
	const auto image_data = buffer->buffer();

	stage_elapsed.reset();
	const auto write_success = writeCacheFile(makeImageCachePath(tag_id), image_data);
	const auto write_elapsed = stage_elapsed.elapsedSeconds();
	if (!write_success) {
		XAMP_LOG_DEBUG("Failure to save image cache. ({})", makeImageCachePath(tag_id).toStdString());
	}
	const auto image_size_text = qFormat("%1x%2")
		.arg(resize_image.width())
		.arg(resize_image.height())
		.toStdString();

	if (save_only) {
		XAMP_LOG_D(logger_, "Add image cache save only. cover:{} size:{} bytes:{} resize:{:.3f}s encode:{:.3f}s write:{:.3f}s total:{:.3f}s",
			tag_id.toStdString(),
			image_size_text,
			image_data.size(),
			resize_elapsed,
			encode_elapsed,
			write_elapsed,
			total_elapsed.elapsedSeconds());
		resetBuffer(*buffer);
		return tag_id;
	}
	
	stage_elapsed.reset();
	cache_.addOrUpdate(tag_id, { buffer->size(), resize_image });
	const auto cache_update_elapsed = stage_elapsed.elapsedSeconds();

	resetBuffer(*buffer);

	XAMP_LOG_D(logger_, "Add image cache. cover:{} size:{} bytes:{} resize:{:.3f}s encode:{:.3f}s write:{:.3f}s cache_update:{:.3f}s total:{:.3f}s",
		tag_id.toStdString(),
		image_size_text,
		image_data.size(),
		resize_elapsed,
		encode_elapsed,
		write_elapsed,
		cache_update_elapsed,
		total_elapsed.elapsedSeconds());
	return tag_id;
}

void ImageCache::loadCache() const {
	qDaoFacade.album_dao.forEachAlbumCover([this](const QString& cover_id) {
		(void) getOrAddDefault(cover_id);
		});
}


bool ImageCache::isFileExists(const QString& cover_id) const {
	QFileInfo file_info = getImageFileInfo(cover_id);
	return file_info.exists();
}

bool ImageCache::contains(const QString& tag_id) const {
	if (tag_id.isEmpty()) {
		return false;
	}
	return cache_.contains(tag_id);
}

std::optional<QPixmap> ImageCache::tryGet(const QString& cover_id) const {
	XAMP_LOG_T(logger_, "cover:{} cache-size: {}, cache: {}",
		cover_id.toStdString(),
		String::formatBytes(cache_.getSize()), cache_);

	if (cover_id.isEmpty()) {
		return std::nullopt;
	}

	ImageCacheEntity entity;
	if (cache_.tryGet(cover_id, entity)) {
		return makeOptional<QPixmap>(entity.image);
	}
	return std::nullopt;
}

QPixmap ImageCache::getOrDefault(const QString& cover_id) {
	XAMP_LOG_T(logger_, "tag:{} cache: {}",
		String::formatBytes(cache_.getSize()), cache_);

	if (cover_id.isEmpty()) {
		return qTheme.defaultSizeUnknownCover();
	}

	if (cover_id == unknownCoverId()) {
		return getOrAddDefault(cover_id);
	}

	if (auto cache_cover = tryGet(cover_id)) {
		return cache_cover.value();
	}

	auto entity = getFromFile(cover_id);
	if (entity.image.isNull()) {
		return qTheme.defaultSizeUnknownCover();
	}

	addOrUpdateCover(cover_id, entity.image);
	if (auto cache_cover = tryGet(cover_id)) {
		return cache_cover.value();
	}
	return entity.image;
}

QPixmap ImageCache::getOrAddDefault(const QString& cover_id, bool not_found_use_default) const {
	const auto [size, image] = cache_.getOrAdd(cover_id, [cover_id, this]() {
		XAMP_LOG_D(logger_, "Load cover:{}", cover_id.toStdString());
		return getFromFile(cover_id);
	});

	if (!cover_id.isEmpty()) {
		XAMP_LOG_D(logger_, "Find cover:{} {}", cover_id.toStdString(), cache_);
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
	return cache_.getSize();
}

void ImageCache::timerEvent(QTimerEvent* ) {
	if (cache_.getSize() > trim_target_size_) {
		cache_.evict(trim_target_size_);
	}
	XAMP_LOG_T(logger_, "Trim target-cache-size: {}, cache: {}", 
		String::formatBytes(trim_target_size_), cache_);
}

QIcon uniformIcon(const QIcon& icon, QSize size) {
	QIcon result;
	const auto base_pixmap = icon.pixmap(size);
	for (const auto state : { QIcon::Off, QIcon::On }) {
		for (const auto mode : { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected })
			result.addPixmap(base_pixmap, mode, state);
	}
	return result;
}

IconCache::IconCache() {
}

QIcon IconCache::getOrAddIcon(const QString& id) const {
	const QIcon icon(image_util::roundImage(qImageCache.getOrAddDefault(id), kCoverSize));
	return uniformIcon(icon, kCoverSize);
}
