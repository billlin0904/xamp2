//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStringList>

#include "playlistentity.h"

class PixmapCache {
public:
	static PixmapCache& Instance() {
		static PixmapCache instance;
		return instance;
	}

    static QPixmap findDirExistCover(const QString &file_path);

	static QPixmap findDirExistCover(const PlayListEntity &item);

    bool find(const QString &tag_id, QPixmap &cover) const;

	const QPixmap* find(const QString& tag_id) const;

	bool insert(const QPixmap& cover, QString* cover_tag_id = nullptr);

	void erase(const QString &tag_id);

	bool exist(const QString& cover_id) const;

protected:
	PixmapCache();

private:
	QString savePixmap(const QPixmap& pixmap) const;

    void loadCache() const;

	QString cache_path_;
    QStringList cover_ext_;
    QStringList cache_ext_;
};
