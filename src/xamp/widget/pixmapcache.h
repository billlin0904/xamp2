//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStringList>

#include "lrucache.h"
#include "playlistentity.h"

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

class PixmapCache {
public:
	static PixmapCache& Instance() {
		static PixmapCache instance;
		return instance;
	}

    static QPixmap findDirExistCover(const QString &file_path);

	static QPixmap findDirExistCover(const PlayListEntity &item);

	const QPixmap* find(const QString& tag_id) const;

	bool insert(const QPixmap& cover, QString* cover_tag_id = nullptr);

	void erase(const QString& tag_id);

protected:
	PixmapCache();

private:
	QString savePixmap(const QPixmap& pixmap) const;

    void loadCache() const;

	QString cache_path_;
    QStringList cover_ext_;
    QStringList cache_ext_;
	mutable LruCache<QString, QPixmap> cache_;
};
