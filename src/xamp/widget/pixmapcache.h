//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStringList>
#include <optional>

#include "lrucache.h"
#include "playlistentity.h"

#ifndef QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF
namespace std {
template <>
struct hash<QString> {
	typedef size_t result_type;
	typedef QString argument_type;

	result_type operator()(const argument_type & s) const {
		return qHash(s);
	}
};
}
#endif

class PixmapCache {
public:
	static PixmapCache& Instance() {
		static PixmapCache instance;
		return instance;
	}

    static QPixmap findDirExistCover(const QString &file_path);

	static QPixmap findDirExistCover(const PlayListEntity &item);

	std::optional<const QPixmap*> find(const QString& tag_id) const;

    QPixmap fromFileCache(const QString& tag_id) const;

    QString emplace(const QPixmap& cover) const;

	void erase(const QString& tag_id);

    bool isExist(const QString& tag_id) const;

protected:
	PixmapCache();

private:
    void loadCache() const;

	QString cache_path_;
    QStringList cover_ext_;
    QStringList cache_ext_;
	mutable LruCache<QString, QPixmap> cache_;
};
