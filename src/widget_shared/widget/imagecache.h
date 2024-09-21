//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

struct XAMP_WIDGET_SHARED_EXPORT ImageCacheEntity {
	ImageCacheEntity(int64_t size = 0, const QPixmap &image = QPixmap())
		: size(size)
		, image(image) {
	}
	int64_t size;
	QPixmap image;
};

struct ImageCacheSizeOfPolicy {
	int64_t operator()(const QString&, const ImageCacheEntity& entity) const noexcept {
		return entity.size;
	}
};

class QTimerEvent;

inline constexpr ConstexprQString kAlbumCacheTag("album_thumbnail_"_str);

class XAMP_WIDGET_SHARED_EXPORT ImageCache final : public QObject {
public:
	static constexpr char kImageFileFormat[] = "PNG";
	static constexpr int kTrimImageSizeSeconds = 10 * 1000;
	static constexpr QImage::Format kImageFormat = QImage::Format_RGB888;
	static constexpr auto kCoverSize = QSize(38, 38);

    friend class SharedSingleton<ImageCache>;

	void loadUnknownCover();

	QPixmap scanCoverFromDir(const QString& file_path);

	QPixmap findImageFromDir(const PlayListEntity& item);

    QPixmap getOrAddDefault(const QString& tag_id, bool not_found_use_default = true) const;

	ImageCacheEntity getFromFile(const QString& tag_id) const;

	void removeImage(const QString& tag_id) const;

	size_t size() const;

    void setMaxSize(size_t max_size);

	QString unknownCoverId() const {
		return unknown_cover_id_;
	}

	void clear() const;

	void clearCache() const;

	QString addImage(const QPixmap& cover, bool save_only = false);

	QPixmap getOrAdd(const QString& tag_id, std::function<QPixmap()>&& value_factory);

	QPixmap getOrDefault(const QString& tag, const QString& cover_id);

	void remove(const QString& cover_id);

	void addOrUpdateCover(const QString& tag, const QString& cover_id, const QPixmap& cover);

	QIcon getOrAddIcon(const QString& id) const;

	void addOrUpdateIcon(const QString& id, const QIcon &value) const;

	QIcon uniformIcon(const QIcon &icon, QSize size) const;

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

protected:
	ImageCache();

private:
	void timerEvent(QTimerEvent*) override;

	void loadCache() const;	

	QString makeCacheFilePath() const;

	QString makeImageCachePath(const QString& tag_id) const;

	QFileInfo getImageFileInfo(const QString& tag_id) const;

	QStringList cover_ext_;
	QStringList cache_ext_;

	int32_t trim_target_size_;
	QString unknown_cover_id_;
	LoggerPtr logger_;
	mutable LruCache<QString, ImageCacheEntity, ImageCacheSizeOfPolicy> cache_;
	mutable std::shared_ptr<ObjectPool<QBuffer>> buffer_pool_;
	mutable LruCache<QString, QPixmap> cover_cache_;
};

#define qImageCache SharedSingleton<ImageCache>::GetInstance()
