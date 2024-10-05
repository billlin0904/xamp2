#include <widget/youtubedl/ytmusicservice.h>
#include <widget/util/json_util.h>

#include <QPixmap>

namespace {
	constexpr auto BASE_URL = "http://127.0.0.1:8080"_str;

	QList<QString> extractSuggestions(const QJsonDocument& json_doc) {
		QList<QString> suggestions_list;
		if (json_doc.isObject()) {
			QJsonObject json_obj = json_doc.object();
			if (json_obj.contains("suggestions"_str) && json_obj["suggestions"_str].isArray()) {
				QJsonArray suggestions_array = json_obj["suggestions"_str].toArray();
				for (const QJsonValue& value : suggestions_array) {
					if (value.isString()) {
						suggestions_list.append(value.toString());
					}
				}
			}
		}
		return suggestions_list;
	}

	std::vector<meta::Artist> parseArtists(const QJsonArray& artists_array) {
		std::vector<meta::Artist> artists;
		for (const QJsonValue& artist_value : artists_array) {
			QJsonObject artist_obj = artist_value.toObject();
			meta::Artist artist;
			artist.name = artist_obj["name"_str].toString().toStdString();
			if (artist_obj.contains("id"_str)) {
				artist.id = artist_obj["id"_str].toString().toStdString();
			}
			artists.push_back(artist);
		}
		return artists;
	}

	std::vector<meta::Thumbnail> parseThumbnails(const QJsonArray& thumbnails_array) {
		std::vector<meta::Thumbnail> thumbnails;
		for (const QJsonValue& thumbnail_value : thumbnails_array) {
			QJsonObject thumbnail_obj = thumbnail_value.toObject();
			meta::Thumbnail thumbnail;
			thumbnail.url = thumbnail_obj["url"_str].toString().toStdString();
			thumbnail.width = thumbnail_obj["width"_str].toInt();
			thumbnail.height = thumbnail_obj["height"_str].toInt();
			thumbnails.push_back(thumbnail);
		}
		return thumbnails;
	}

	std::vector<album::Track> parseTracks(const QJsonArray& tracksArray) {
		std::vector<album::Track> tracks;
		for (const QJsonValue& trackValue : tracksArray) {
			QJsonObject trackObj = trackValue.toObject();
			album::Track track;
			track.title = trackObj["title"_str].toString().toStdString();
			if (trackObj.contains("isExplicit"_str)) {
				track.is_explicit = trackObj["isExplicit"_str].toBool();
			}
			if (trackObj.contains("album"_str)) {
				track.album = trackObj["album"_str].toString().toStdString();
			}
			if (trackObj.contains("videoId"_str)) {
				track.video_id = trackObj["videoId"_str].toString().toStdString();
			}
			if (trackObj.contains("duration"_str)) {
				track.duration = trackObj["duration"_str].toString().toStdString();
			}
			if (trackObj.contains("likeStatus"_str)) {
				track.like_status = trackObj["likeStatus"_str].toString().toStdString();
			}
			// 解析 artists
			if (trackObj.contains("artists"_str) && trackObj["artists"_str].isArray()) {
				track.artists = parseArtists(trackObj["artists"_str].toArray());
			}
			tracks.push_back(track);
		}
		return tracks;
	}

	album::Album parseAlbum(const QJsonObject& album_obj) {
		album::Album album;

		album.title = album_obj["title"_str].toString().toStdString();
		album.track_count = album_obj["trackCount"_str].toInt();
		album.duration = album_obj["duration"_str].toString().toStdString();

		if (album_obj.contains("audioPlaylistId"_str)) {
			album.audio_playlist_id = album_obj["audioPlaylistId"_str].toString().toStdString();
		}
		if (album_obj.contains("year"_str)) {
			album.year = album_obj["year"_str].toString().toStdString();
		}
		if (album_obj.contains("description"_str)) {
			album.description = album_obj["description"_str].toString().toStdString();
		}

		// 解析 artists
		if (album_obj.contains("artists"_str) && album_obj["artists"_str].isArray()) {
			album.artists = parseArtists(album_obj["artists"_str].toArray());
		}

		// 解析 thumbnails
		if (album_obj.contains("thumbnails"_str) && album_obj["thumbnails"_str].isArray()) {
			album.thumbnails = parseThumbnails(album_obj["thumbnails"_str].toArray());
		}

		// 解析 tracks
		if (album_obj.contains("tracks"_str) && album_obj["tracks"_str].isArray()) {
			album.tracks = parseTracks(album_obj["tracks"_str].toArray());
		}

		return album;
	}

	search::Album parseSearchAlbum(const QJsonObject& album_obj) {
		search::Album album;

		if (album_obj.contains("browseId"_str)) {
			album.browse_id = album_obj["browseId"_str].toString().toStdString();
		}

		album.title = album_obj["title"_str].toString().toStdString();
		album.type = album_obj["type"_str].toString().toStdString();
		if (album_obj.contains("year"_str)) {
			album.year = album_obj["year"_str].toString().toStdString();
		}

		album.is_explicit = album_obj["is_explicit"_str].toBool();

		if (album_obj.contains("artists"_str) && album_obj["artists"_str].isArray()) {
			album.artists = parseArtists(album_obj["artists"_str].toArray());
		}

		if (album_obj.contains("thumbnails"_str) && album_obj["thumbnails"_str].isArray()) {
			album.thumbnails = parseThumbnails(album_obj["thumbnails"_str].toArray());
		}

		return album;
	}

	template <typename T>
	void parsePlaylistJson(LibraryPlaylist& playlist, const T& playlist_obj) {
		// 解析 "title" 和 "playlistId"
		QString title = playlist_obj["title"_str].toString();
		QString playlist_id = playlist_obj["playlistId"_str].toString();

		playlist.title = title;
		playlist.playlist_id = playlist_id;

		// 解析 "tracks" 陣列
		QJsonArray tracks_array = playlist_obj["tracks"_str].toArray();
		for (const QJsonValue& track_value : tracks_array) {
			playlist::Track track;

			QJsonObject track_obj = track_value.toObject();

			// 解析 track 的相關資訊
			QString video_id = track_obj["videoId"_str].toString();
			QString track_title = track_obj["title"_str].toString();
			QString like_status = track_obj["likeStatus"_str].toString();
			QString duration = track_obj["duration"_str].toString();

			track.video_id = video_id.toStdString();
			track.title = track_title.toStdString();
			track.like_status = like_status.toStdString();
			track.duration = duration.toStdString();
			track.set_video_id = track_obj["setVideoId"_str].toString().toStdString();

			// 解析 "artists" 陣列
			QJsonArray artists_array = track_obj["artists"_str].toArray();
			for (const QJsonValue& artist_value : artists_array) {
				QJsonObject artistObj = artist_value.toObject();
				QString artistName = artistObj["name"_str].toString();

				meta::Artist artist;
				artist.name = artistName.toStdString();
				track.artists.push_back(artist);
			}

			// 解析 "album" 物件
			QJsonObject album_obj = track_obj["album"_str].toObject();
			QString album_name = album_obj["name"_str].toString();
			meta::Album album;
			album.name = album_name.toStdString();
			track.album = album;

			// 解析 "thumbnails" 陣列
			QJsonArray thumbnails_array = track_obj["thumbnails"_str].toArray();
			for (const QJsonValue& thumbnail_value : thumbnails_array) {
				QJsonObject thumbnail_obj = thumbnail_value.toObject();
				QString thumbnail_url = thumbnail_obj["url"_str].toString();
				int width = thumbnail_obj["width"_str].toInt();
				int height = thumbnail_obj["height"_str].toInt();
				meta::Thumbnail thumbnail;
				thumbnail.url = thumbnail_url.toStdString();
				thumbnail.width = width;
				thumbnail.height = height;
				track.thumbnails.push_back(thumbnail);
			}

			playlist.tracks.push_back(track);
		}
	}
}



YtMusicHttpService::YtMusicHttpService()
	: http_client_(BASE_URL) {
}

QCoro::Task<SongInfo> YtMusicHttpService::fetchSongInfo(const QString& video_id) {
    http_client_.setUrl(qFormat("%1/fetch_song_info").arg(BASE_URL));

	QVariantMap map;
	map["video_id"_str] = video_id;
	http_client_.setJson(json_util::serialize(map));

	SongInfo song_info;

    auto content = co_await http_client_.post();

    QJsonDocument doc;
    if (!json_util::deserialize(content, doc)) {
        co_return song_info;
    }

    const QString download_url = doc["download_url"_str].toString();
    const QString thumbnail_base64 = doc["thumbnail_base64"_str].toString();
	const QString lyrics = doc["lyrics"_str].toString();

    song_info.download_url = download_url;
    song_info.thumbnail_base64 = thumbnail_base64;
	song_info.lyrics = lyrics;
    co_return song_info;
}

QCoro::Task<QList<LibraryPlaylist>> YtMusicHttpService::fetchLibraryPlaylists() {
    http_client_.setUrl(qFormat("%1/fetch_library_playlists").arg(BASE_URL));
    http_client_.addAcceptJsonHeader();
    auto content = co_await http_client_.get();

    QList<LibraryPlaylist> results;
    QJsonDocument doc;
    if (!json_util::deserialize(content, doc)) {
        co_return results;
    }

    const auto json_array = doc.array();

    // 遍歷 JSON 陣列中的物件
    for (const QJsonValue& value : json_array) {
		LibraryPlaylist playlist;
        QJsonObject playlist_obj = value.toObject();
        parsePlaylistJson(playlist, playlist_obj);
        results.push_back(playlist);
    }

    co_return results;
}

QCoro::Task<QString> YtMusicHttpService::fetchLyrics(const QString& video_id) {
    http_client_.setUrl(qFormat("%1/fetch_lyrics").arg(BASE_URL));

	QVariantMap map;
	map["video_id"_str] = video_id;
	http_client_.setJson(json_util::serialize(map));

    auto content = co_await http_client_.post();

    QJsonDocument doc;
    if (!json_util::deserialize(content, doc)) {
        co_return QString();
    }

    QString _video_id = doc["video_id"_str].toString();
    QString lyrics = doc["lyrics"_str].toString();
    co_return lyrics;
}

QCoro::Task<QList<search::Album>> YtMusicHttpService::searchAlbum(const QString& query) {
	http_client_.setUrl(qFormat("%1/search_album").arg(BASE_URL));

	QVariantMap map;
	map["query"_str] = query;
	http_client_.setJson(json_util::serialize(map));

	auto content = co_await http_client_.post();

	QJsonDocument doc;
	QList<search::Album> result;

	if (!json_util::deserialize(content, doc)) {
		co_return result;
	}

	auto albums = doc["albums"_str].toArray();
	for (const QJsonValue& value : albums) {
		result.append(parseSearchAlbum(value.toObject()));
	}

	co_return result;
}

QCoro::Task<album::Album> YtMusicHttpService::fetchAlbum(const QString& browse_id) {
	http_client_.setUrl(qFormat("%1/get_album").arg(BASE_URL));

	QVariantMap map;
	map["browse_id"_str] = browse_id;
	http_client_.setJson(json_util::serialize(map));

	auto content = co_await http_client_.post();
	album::Album album;
	QJsonDocument doc;
	if (!json_util::deserialize(content, doc)) {
		co_return album;
	}
	album = parseAlbum(doc.object());
	co_return album;
}

QCoro::Task<QList<QString>> YtMusicHttpService::searchSuggestions(const QString& query) {
	http_client_.setUrl(qFormat("%1/search_suggestions").arg(BASE_URL));

	QVariantMap map;
	map["query"_str] = query;
	http_client_.setJson(json_util::serialize(map));

	auto content = co_await http_client_.post();
	QJsonDocument doc;
	QList<QString> suggestions;
	if (!json_util::deserialize(content, doc)) {
		co_return suggestions;
	}

	suggestions = extractSuggestions(doc);
	co_return suggestions;
}

QCoro::Task<LibraryPlaylist> YtMusicHttpService::fetchPlaylist(const QString& playlist_id) {
    http_client_.setUrl(qFormat("%1/fetch_playlist").arg(BASE_URL));

	QVariantMap map;
	map["playlist_id"_str] = playlist_id;
	const auto json = json_util::serialize(map);
    http_client_.setJson(json);

    auto content = co_await http_client_.post();

    LibraryPlaylist playlist;
    QJsonDocument doc;
    if (!json_util::deserialize(content, doc)) {
        co_return playlist;
    }

    parsePlaylistJson(playlist, doc);
    co_return playlist;
}

QCoro::Task<QString> YtMusicHttpService::editPlaylist(const QString& playlist_id, const QString& title) {
	http_client_.setUrl(qFormat("%1/edit_playlist").arg(BASE_URL));

	QVariantMap map;
	map["playlist_id"_str] = playlist_id;
	map["new_title"_str] = title;
	const auto json = json_util::serialize(map);

	http_client_.setJson(json);
	auto content = co_await http_client_.post();
	co_return content;
}

QCoro::Task<QString> YtMusicHttpService::deletePlaylist(const QString& playlist_id) {
	http_client_.setUrl(qFormat("%1/delete_playlist").arg(BASE_URL));

	QVariantMap map;
	map["playlist_id"_str] = playlist_id;
	const auto json = json_util::serialize(map);

	http_client_.setJson(json);
	auto content = co_await http_client_.post();
	co_return content;
}

QCoro::Task<QString> YtMusicHttpService::createPlaylist(const QString& title,
	const QString& description,
	PrivateStatus status,
	const QList<QString>& video_ids,
	const QString& source_playlist) {
	http_client_.setUrl(qFormat("%1/create_playlist").arg(BASE_URL));

	QVariantMap map;
	map["title"_str] = title;
	map["description"_str] = description;
	map["privacy_status"_str] = QString::fromStdString(std::string(EnumToString(status)));
	map["video_ids"_str] = video_ids;
	map["source_playlist"_str] = source_playlist;
	const auto json = json_util::serialize(map);

	http_client_.setJson(json);
	auto content = co_await http_client_.post();
	co_return content;
}
