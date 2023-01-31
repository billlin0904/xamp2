#include <QJsonArray>
#include <QJsonDocument>

#include <widget/str_utilts.h>
#include <widget/spotify_utilis.h>

namespace spotify {

bool ParseSearchLyricsResult(QString const& json, QList<SearchLyricsResult>& results) {
	QJsonParseError error;
	SearchLyricsResult result;

	const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError) {
		return false;
	}
	auto songs = doc[qTEXT("result")][qTEXT("songs")].toArray();
	for (const auto entry : songs) {
		auto object = entry.toVariant().toMap();
		result.id = object.value(qTEXT("id")).toInt();
		result.song = object.value(qTEXT("name")).toString();
		results.push_back(result);
	}

	return !results.isEmpty();
}

std::tuple<QString, QString> ParseLyricsResponse(QString const& json) {
	QJsonParseError error;
	const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError) {
		return { qEmptyString, qEmptyString };
	}
	auto lyrics = doc[qTEXT("lrc")][qTEXT("lyric")].toString();
	auto trlyrics = doc[qTEXT("tlyric")][qTEXT("lyric")].toString();
	return { lyrics, trlyrics };
}

}
