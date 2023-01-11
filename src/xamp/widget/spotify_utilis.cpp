#include <QJsonArray>
#include <QJsonDocument>

#include <widget/str_utilts.h>
#include <widget/spotify_utilis.h>

namespace Spotify {

void parseJson(QString const& json, SearchLyricsResult& result) {
	QJsonParseError error;

	const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
	if (error.error == QJsonParseError::NoError) {
		auto songs = doc[qTEXT("result")][qTEXT("songs")].toArray();
		for (const auto entry : songs) {
			auto object = entry.toVariant().toMap();
			auto id = object.value(qTEXT("id")).toInt();
		}
	}
}

}
