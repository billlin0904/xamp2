#include <widget/musixmatchclient.h>

inline ConstLatin1String kAPIKey = Q_UTF8("");
inline ConstLatin1String kUrl = Q_UTF8("http://api.musixmatch.com/ws/1.1/");

MusixmatchClient::MusixmatchClient(QNetworkAccessManager* manager, QObject* parent)
	: QObject(parent)
	, manager_(manager) {
}

QString MusixmatchClient::getUrl(QString const& url) const {
	return QString(Q_UTF8("%1%2&apikey=%3")).arg(kUrl)
	.arg(url)
	.arg(kAPIKey);
}

void MusixmatchClient::matcherLyrics(QString const& q_track, QString const& q_artist, QString format) {
	const auto url = getUrl(
		Q_UTF8("matcher.lyrics.get?") +
		Q_STR("q_track=%1&q_artist=%2&format=%3").arg(q_track).arg(q_artist).arg(format));

	auto handler = [=](const QString& msg) {
	};

	http::HttpClient(url, manager_)
		.success(handler)
		.get();
}

void MusixmatchClient::chartArtists(QString const& page, uint32_t page_size, QString country, QString format) {
}

void MusixmatchClient::chartTrack(QString const& page, uint32_t page_size, bool f_has_lyrics, QString country,
	QString format) {
	auto handler = [=](const QString& msg) {
	};
}

void MusixmatchClient::trackSearch(QString const& q_track, QString const& q_artist, uint32_t page_size,
	QString const& page, uint32_t s_track_rating, QString format) {
}

void MusixmatchClient::track(QString const& track_id, QString const& commontrack_id, QString const& track_isrc,
	QString const& track_mbid, QString format) {
}

void MusixmatchClient::trackLyrics(QString const& track_id, QString const& commontrack_id, QString format)
{
}

void MusixmatchClient::trackSnippet(QString const& track_id, QString format)
{
}

void MusixmatchClient::trackSubtitle(QString const& track_id, QString const& track_mbid, QString const& subtitle_format,
	QString const& f_subtitle_length, QString const& f_subtitle_length_max_deviation, QString format)
{
}
