//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMetaType>
#include <QString>
#include <QList>

#include <widget/widget_shared_global.h>

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

bool ParseSearchLyricsResult(QString const& json, QList<SearchLyricsResult> & results);

std::tuple<QString, QString> ParseLyricsResponse(QString const& json);

}

Q_DECLARE_METATYPE(spotify::SearchLyricsResult)
