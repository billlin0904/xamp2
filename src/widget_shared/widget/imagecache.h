//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFileInfo>
#include <QObject>
#include <QBuffer>

#include <base/logger.h>
#include <base/object_pool.h>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/image_utiltis.h>
#include <widget/widget_shared_global.h>

struct ImageCacheEntity {
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

class XAMP_WIDGET_SHARED_EXPORT ImageCache final : public QObject {
public:
	static constexpr char kImageFileFormat[] = "PNG";
	static constexpr int kTrimImageSizeSeconds = 3 * 1000;

    friend class SharedSingleton<ImageCache>;

	static QPixmap ScanCoverFromDir(const QString& file_path);

	static QPixmap FindImageFromDir(const PlayListEntity& item);

    QPixmap GetOrDefault(const QString& tag_id, bool not_found_use_default = true) const;

	ImageCacheEntity GetFromFile(const QString& tag_id) const;

	void RemoveImage(const QString& tag_id) const;

	size_t GetSize() const;

    void SetMaxSize(size_t max_size);

	QString GetUnknownCoverId() const {
		return unknown_cover_id_;
	}

	void Clear() const;

	void ClearCache() const;

	QString AddImage(const QPixmap& cover) const;

	QPixmap GetOrAdd(const QString& tag_id, std::function<QPixmap()>&& value_factory) const;

	QPixmap GetCover(const QString& tag, const QString& cover_id);
protected:
	ImageCache();

private:
	void OptimizeImageFromBuffer(const QString& file_path, const QByteArray& buffer, const QString& tag_name) const;

	void timerEvent(QTimerEvent*) override;

	void LoadCache() const;

	void InitCachePath();

	QFileInfo GetImageFileInfo(const QString& tag_id) const;

	static QStringList cover_ext_;
	static QStringList cache_ext_;

	int32_t trim_target_size_;
	QString unknown_cover_id_;
	QString cache_path_;
	LoggerPtr logger_;
	mutable LruCache<QString, ImageCacheEntity, ImageCacheSizeOfPolicy> cache_;
	mutable std::shared_ptr<ObjectPool<QBuffer>> buffer_pool_;
	LruCache<QString, QPixmap> cover_cache_;
};

#define qPixmapCache SharedSingleton<ImageCache>::GetInstance()
