//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStringList>
#include <QBuffer>

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

class PixmapCache final {
public:
    friend class SharedSingleton<PixmapCache>;

	static QPixmap findFileDirCover(const QString& file_path);

	static QPixmap findFileDirCover(const PlayListEntity& item);

    const QPixmap* find(const QString& tag_id) const;

    QPixmap fromFileCache(const QString& tag_id) const;

    QString addOrUpdate(const QByteArray& data) const;

	void erase(const QString& tag_id);

	bool isExist(const QString& tag_id) const;

	size_t totalCacheSize() const;

    void setMaxSize(size_t max_size);

	QString getUnknownCoverId() const {
		return unknown_cover_id_;
	}

	void clear();

	void clearCache();

    size_t missRate() const;

	QString savePixamp(const QPixmap& cover) const;

protected:
	PixmapCache();

private:
	void loadCache() const;

	QString unknown_cover_id_;
	QString cache_path_;
    static QStringList cover_ext_;
    static QStringList cache_ext_;
	mutable LruCache<QString, QPixmap> cache_;
	std::shared_ptr<spdlog::logger> logger_;
};

#define qPixmapCache SharedSingleton<PixmapCache>::GetInstance()