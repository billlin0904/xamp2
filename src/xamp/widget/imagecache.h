//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFileInfo>
#include <QObject>

#include <base/logger.h>
#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/image_utiltis.h>

#ifndef QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF
namespace std {
	template <>
	struct hash<QString> {
		typedef size_t result_type;
		typedef QString argument_type;

		result_type operator()(const argument_type& s) const {
			return qHash(s);
		}
	};
}
#endif

struct ImageCacheEntity {
	int64_t size;
	QPixmap image;
};

struct ImageCacheSizeOfPolicy {
	int64_t operator()(const QString&, const ImageCacheEntity& entity) {
		return entity.size;
	}
};

class QTimerEvent;

class ImageCache final : public QObject {
public:
	static constexpr char kImageFileFormat[] = "PNG";
	static constexpr int kTrimImageSizeSeconds = 10 * 1000;

    friend class SharedSingleton<ImageCache>;

	static QPixmap ScanImageFromDir(const QString& file_path);

	static QPixmap FindImageFromDir(const PlayListEntity& item);

    QPixmap GetOrDefault(const QString& tag_id, bool not_found_use_default = true) const;

	ImageCacheEntity GetFromFile(const QString& tag_id) const;

	void RemoveImage(const QString& tag_id);

	size_t GetSize() const;

    void SetMaxSize(size_t max_size);

	QString GetUnknownCoverId() const {
		return unknown_cover_id_;
	}

	void Clear();

	void ClearCache();

	QString AddImage(const QPixmap& cover) const;

protected:
	ImageCache();

private:
	void OptimizeImageInDir(const QString& file_path) const;

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
};

#define qPixmapCache SharedSingleton<ImageCache>::GetInstance()