//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QList>
#include <QMetaType>

namespace spotify {

struct SearchLyricsResult {
	struct Artist {
		int32_t id{};
		QString name;
		QString img1v1_url;
	};

	struct Album {
		int32_t id{};
		int32_t pic_id{};
		QString name;
		QString img1v1_url;
	};

	int32_t id{};
	QString song;
	Album album;
	QList<Artist> artists;
};

Q_DECLARE_METATYPE(SearchLyricsResult)

bool ParseSearchLyricsResult(QString const& json, QList<SearchLyricsResult> & results);

QString ParseLyricsResponse(QString const& json);

}

