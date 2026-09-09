//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFileInfo>
#include <QObject>
#include <QBuffer>

#include <base/logger.h>
#include <base/object_pool.h>

#include <widget/themecolor.h>
#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/util/image_util.h>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_API ImageCacheEntity {
	ImageCacheEntity(int64_t size = 0, const QPixmap &image = QPixmap())
		: size(size)
		, image(image) {
	}
	int64_t size;
	QPixmap image;
};

struct ImageCacheSizeOfPolicy {
	int64_t operator()(const QString&, const ImageCacheEntity& entity) const {
		return entity.size;
	}
};

class QTimerEvent;

inline constexpr ConstexprQString kAlbumCacheTag("album_thumbnail_"_str);

inline constexpr auto kCoverSize = QSize(38, 38);

class XAMP_WIDGET_SHARED_API ImageCache final : public QObject {
public:
	static constexpr char kImageFileFormat[] = "PNG";
	static constexpr int kTrimImageSizeSeconds = 10 * 1000;
	static constexpr QImage::Format kImageFormat = QImage::Format_RGB888;

	XAMP_DECLARE_SINGLETON_NAME()

	ImageCache();

	void loadUnknownCover();

	bool contains(const QString& tag_id) const;

	bool isFileExists(const QString& cover_id) const;    

	size_t size() const;

    void setMaxSize(size_t max_size);

	QString unknownCoverId() const {
		return unknown_cover_id_;
	}

	void clearCache() const;

	void removeCoverId(const QString& cover_id) const;

	ImageCacheEntity getFromFile(const QString& cover_id) const;

	QString addImage(const QPixmap& cover, bool save_only = false);

	QPixmap getOrAddDefault(const QString& cover_id, bool not_found_use_default = true) const;

	std::optional<QPixmap> tryGet(const QString& cover_id) const;

	QPixmap getOrDefault(const QString& cover_id);	

public slots:

private:
	void timerEvent(QTimerEvent*) override;

	void loadCache() const;	

	bool saveCacheImage(const QString& cover_id, const QPixmap& image, bool update_memory, qint64* encoded_size = nullptr) const;

	void addOrUpdateCover(const QString& cover_id, const QPixmap& cover) const;

	int64_t trim_target_size_;
	QStringList cover_ext_;
	QStringList cache_ext_;	
	QString unknown_cover_id_;
	LoggerPtr logger_;
	mutable LruCache<QString, ImageCacheEntity, ImageCacheSizeOfPolicy> cache_;
	mutable std::shared_ptr<ObjectPool<QBuffer>> buffer_pool_;
};

#define qImageCache SharedSingleton<ImageCache>::getInstance()

XAMP_WIDGET_SHARED_API QIcon uniformIcon(const QIcon& icon, QSize size);

class IconCache {
public:
	IconCache();

	QIcon getOrAddIcon(const QString& id) const;	
};

#define qIconCache SharedSingleton<IconCache>::getInstance()
