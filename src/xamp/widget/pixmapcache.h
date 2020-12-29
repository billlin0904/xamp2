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

		result_type operator()(const argument_type& s) const {
			return qHash(s);
		}
	};
}
#endif

class PixmapCache final {
public:
	friend class Singleton<PixmapCache>;

	static QPixmap findFileDirCover(const QString& file_path);

	static QPixmap findFileDirCover(const PlayListEntity& item);

	std::optional<const QPixmap*> find(const QString& tag_id) const;

	QPixmap fromFileCache(const QString& tag_id) const;

	QString addOrUpdate(const QPixmap& cover) const;

	void erase(const QString& tag_id);

	bool isExist(const QString& tag_id) const;

	size_t getImageSize() const;

	QString getUnknownCoverId() const {
		return unknown_cover_id_;
	}

protected:
	PixmapCache();

private:
	void loadCache() const;

	void clear();

	QString unknown_cover_id_;
	QString cache_path_;
	QStringList cover_ext_;
	QStringList cache_ext_;
	mutable LruCache<QString, QPixmap> cache_;
};
