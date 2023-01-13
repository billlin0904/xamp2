//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStringList>
#include <QBuffer>
#include <QPixmap>

#include <base/logger.h>
#include <widget/widget_shared.h>
#include <widget/playlistentity.h>

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

class PixmapCache final : public QObject {
	Q_OBJECT
public:
	static constexpr char kCacheFileFormat[] = "PNG";

    friend class SharedSingleton<PixmapCache>;

	static QPixmap findCoverInDir(const QString& file_path);

	static QPixmap findCoverInDir(const PlayListEntity& item);

    const QPixmap* find(const QString& tag_id) const;

    QPixmap fromFileCache(const QString& tag_id) const;

    QString addOrUpdate(const QByteArray& data);

	void addCache(const QString& tag_id, const QPixmap &cover);

	void erase(const QString& tag_id);

	bool isExist(const QString& tag_id) const;

	size_t cacheSize() const;

    void setMaxSize(size_t max_size);

	QString getUnknownCoverId() const {
		return unknown_cover_id_;
	}

	void clear();

	void clearCache();

    size_t missRate() const;

	QString savePixamp(const QPixmap& cover);

	void optimizeImageInDir(const QString& file_path);

	void optimizeImage(const QString& temp_file_path, const QString& file_path, const QString& tag_name);

	void optimizeImageFromBuffer(const QString& file_path, const QByteArray& buffer, const QString& tag_name);
signals:
	void processImage(const QString & file_path, const QByteArray& buffer, const QString& tag_name);

protected:
	PixmapCache();

private:
	void loadCache() const;

	QString unknown_cover_id_;
	QString cache_path_;
    static QStringList cover_ext_;
    static QStringList cache_ext_;
	mutable LruCache<QString, QPixmap> cache_;
	LoggerPtr logger_;
};

#define qPixmapCache SharedSingleton<PixmapCache>::GetInstance()
