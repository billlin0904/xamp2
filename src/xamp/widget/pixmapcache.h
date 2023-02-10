//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
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

struct PixampCacheSizeOfPolicy {
	int32_t operator()(const QString&, const QPixmap& image) {
		return image.height() * image.width() * 4;
	}
};

class QTimerEvent;

class PixmapCache final : public QObject {
	Q_OBJECT
public:
	static constexpr char kImageFileFormat[] = "JPG";

    friend class SharedSingleton<PixmapCache>;

	static QPixmap FindCoverInDir(const QString& file_path);

	static QPixmap FindCoverInDir(const PlayListEntity& item);

    QPixmap find(const QString& tag_id, bool not_found_use_default = true) const;

    QPixmap FromFileCache(const QString& tag_id) const;

    QString AddOrUpdate(const QByteArray& data);

	void AddCache(const QString& tag_id, const QPixmap &cover);

	void erase(const QString& tag_id);

	size_t CacheSize() const;

    void SetMaxSize(size_t max_size);

	QString GetUnknownCoverId() const {
		return unknown_cover_id_;
	}

	void clear();

	void ClearCache();

	QString SavePixamp(const QPixmap& cover);

	void OptimizeImageInDir(const QString& file_path);

	void OptimizeImage(const QString& temp_file_path, const QString& file_path, const QString& tag_name);

	void OptimizeImageFromBuffer(const QString& file_path, const QByteArray& buffer, const QString& tag_name);
signals:
	void ProcessImage(const QString & file_path, const QByteArray& buffer, const QString& tag_name);

public slots:
	

protected:
	PixmapCache();

private:
	void timerEvent(QTimerEvent*) override;

	void LoadCache() const;

	void InitCachePath();

	QFileInfo CacheFileInfo(const QString& tag_id) const;

	static QStringList cover_ext_;
	static QStringList cache_ext_;

	int32_t trim_target_size_;
	QString unknown_cover_id_;
	QString cache_path_;
	LoggerPtr logger_;
	mutable LruCache<QString, QPixmap, PixampCacheSizeOfPolicy> cache_;
};

#define qPixmapCache SharedSingleton<PixmapCache>::GetInstance()
