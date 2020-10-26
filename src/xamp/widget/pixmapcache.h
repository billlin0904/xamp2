//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStringList>
#include <optional>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>

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

class PixmapCache final {
public:
	friend class Singleton<PixmapCache>;
	
    static QPixmap FindFileDirCover(const QString &file_path);

    static QPixmap FindFileDirCover(const PlayListEntity &item);

	std::optional<const QPixmap*> find(const QString& tag_id) const;

    QPixmap FromFileCache(const QString& tag_id) const;

    QString Add(const QPixmap& cover) const;

	void Erase(const QString& tag_id);

    bool IsExist(const QString& tag_id) const;

	size_t GetImageSize() const;

	QString GetUnknownCoverId() const {
		return unknown_cover_id_;
	}

protected:
	PixmapCache();

private:
    void LoadCache() const;

	void Clear();

	QString unknown_cover_id_;
	QString cache_path_;
    QStringList cover_ext_;
    QStringList cache_ext_;
	mutable LruCache<QString, QPixmap> cache_;
};
