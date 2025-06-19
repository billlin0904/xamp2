#include <QPixmap>

#include <widget/worker/ytmusicservice.h>
#include <widget/util/json_util.h>
#include <widget/util/str_util.h>

namespace {
	const QString BASE_URL = "http://127.0.0.1:8090"_str;

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
			artist.name = artist_obj["name"_str].toString();
			if (artist_obj.contains("id"_str)) {
				artist.id = artist_obj["id"_str].toString();
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
			thumbnail.url = thumbnail_obj["url"_str].toString();
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
			track.title = trackObj["title"_str].toString();
			if (trackObj.contains("isExplicit"_str)) {
				track.is_explicit = trackObj["isExplicit"_str].toBool();
			}
			if (trackObj.contains("album"_str)) {
				track.album = trackObj["album"_str].toString();
			}
			if (trackObj.contains("videoId"_str)) {
				track.video_id = trackObj["videoId"_str].toString();
			}
			if (trackObj.contains("duration"_str)) {
				track.duration = trackObj["duration"_str].toString();
			}
			if (trackObj.contains("likeStatus"_str)) {
				track.like_status = trackObj["likeStatus"_str].toString();
			}
			// 解析 artists
			if (trackObj.contains("artists"_str) && trackObj["artists"_str].isArray()) {
				track.artists = parseArtists(trackObj["artists"_str].toArray());
			}
			if (trackObj.contains("trackNumber"_str)) {
				track.track = trackObj["trackNumber"_str].toInt();
			}
			tracks.push_back(track);
		}
		return tracks;
	}

	album::Album parseAlbum(const QJsonObject& album_obj) {
		album::Album album;

		album.title = album_obj["title"_str].toString();
		album.track_count = album_obj["trackCount"_str].toInt();
		album.duration = album_obj["duration"_str].toString();

		if (album_obj.contains("audioPlaylistId"_str)) {
			album.audio_playlist_id = album_obj["audioPlaylistId"_str].toString();
		}
		if (album_obj.contains("year"_str)) {
			album.year = album_obj["year"_str].toString();
		}
		if (album_obj.contains("description"_str)) {
			album.description = album_obj["description"_str].toString();
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
			album.browse_id = album_obj["browseId"_str].toString();
		}

		album.title = album_obj["title"_str].toString();
		album.type = album_obj["type"_str].toString();
		if (album_obj.contains("year"_str)) {
			album.year = album_obj["year"_str].toString();
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

			track.video_id = video_id;
			track.title = track_title;
			track.like_status = like_status;
			track.duration = duration;
			track.set_video_id = track_obj["setVideoId"_str].toString();

			// 解析 "artists" 陣列
			QJsonArray artists_array = track_obj["artists"_str].toArray();
			for (const QJsonValue& artist_value : artists_array) {
				QJsonObject artistObj = artist_value.toObject();
				QString artistName = artistObj["name"_str].toString();
				QString artistId = artistObj["id"_str].toString();
				meta::Artist artist;
				artist.name = artistName;
				artist.id = artistId;
				track.artists.push_back(artist);
			}

			// 解析 "album" 物件
			QJsonObject album_obj = track_obj["album"_str].toObject();
			QString album_name = album_obj["name"_str].toString();
			QString album_id = album_obj["id"_str].toString();
			meta::Album album;
			album.name = album_name;
			album.id = album_id;
			track.album = album;

			// 解析 "thumbnails" 陣列
			QJsonArray thumbnails_array = track_obj["thumbnails"_str].toArray();
			for (const QJsonValue& thumbnail_value : thumbnails_array) {
				QJsonObject thumbnail_obj = thumbnail_value.toObject();
				QString thumbnail_url = thumbnail_obj["url"_str].toString();
				int width = thumbnail_obj["width"_str].toInt();
				int height = thumbnail_obj["height"_str].toInt();
				meta::Thumbnail thumbnail;
				thumbnail.url = thumbnail_url;
				thumbnail.width = width;
				thumbnail.height = height;
				track.thumbnails.push_back(thumbnail);
			}

			playlist.tracks.push_back(track);
		}
	}

	artist::Artist parseArtist(const QJsonObject& artistObj) {
		using namespace artist;

		Artist result;

		// subscribed (bool)
		if (artistObj.contains("subscribed"_str)) {
			result.subscribed = artistObj["subscribed"_str].toBool();
		}

		// description (std::optional<std::string>)
		if (artistObj.contains("description"_str) && !artistObj["description"_str].isNull()) {
			result.description = artistObj["description"_str].toString();
		}

		// views (std::optional<std::string>)
		if (artistObj.contains("views"_str) && !artistObj["views"_str].isNull()) {
			result.views = artistObj["views"_str].toString();
		}

		// name (std::string)
		if (artistObj.contains("name"_str)) {
			result.name = artistObj["name"_str].toString();
		}

		// channel_id (std::string)
		if (artistObj.contains("channel_id"_str)) {
			result.channel_id = artistObj["channel_id"_str].toString();
		}

		// subscribers (std::optional<std::string>)
		if (artistObj.contains("subscribers"_str) && !artistObj["subscribers"_str].isNull()) {
			result.subscribers = artistObj["subscribers"_str].toString();
		}

		// thumbnails (std::vector<meta::Thumbnail>)
		if (artistObj.contains("thumbnails"_str) && artistObj["thumbnails"_str].isArray()) {
			QJsonArray thumbArray = artistObj["thumbnails"_str].toArray();
			result.thumbnails = parseThumbnails(thumbArray);
		}

		auto parseSectionCommon = [&](const QJsonObject& sectionObj,
			auto& section) {
				// browseId
				if (sectionObj.contains("browseId"_str) && !sectionObj["browseId"_str].isNull()) {
					section.browse_id = sectionObj["browseId"_str].toString();
				}

				// params
				if (sectionObj.contains("params"_str) && !sectionObj["params"_str].isNull()) {
					section.params = sectionObj["params"_str].toString();
				}
			};

		if (artistObj.contains("songs"_str) && artistObj["songs"_str].isObject()) {
			QJsonObject songsObj = artistObj["songs"_str].toObject();
			Artist::Section<Artist::Song> songsSection;

			parseSectionCommon(songsObj, songsSection);

			if (songsObj.contains("results"_str) && songsObj["results"_str].isArray()) {
				QJsonArray songArray = songsObj["results"_str].toArray();
				for (const QJsonValue& val : songArray) {
					if (!val.isObject()) continue;
					QJsonObject songObj = val.toObject();

					Artist::Song s;
					// video_id
					if (songObj.contains("videoId"_str)) {
						s.video_id = songObj["videoId"_str].toString();
					}
					// title
					if (songObj.contains("title"_str)) {
						s.title = songObj["title"_str].toString();
					}
					// thumbnails
					if (songObj.contains("thumbnails"_str) && songObj["thumbnails"_str].isArray()) {
						QJsonArray stArr = songObj["thumbnails"_str].toArray();						
						s.thumbnails = parseThumbnails(stArr);
					}
					// artist (vector<meta::Artist>)
					if (songObj.contains("artist"_str) && songObj["artist"_str].isArray()) {
						QJsonArray artArray = songObj["artist"_str].toArray();
						s.artist = parseArtists(artArray);
					}
					// album (Song::Album)
					if (songObj.contains("album"_str) && songObj["album"_str].isObject()) {
						QJsonObject albumObj = songObj["album"_str].toObject();
						Artist::Song::Album alb;
						if (albumObj.contains("name"_str)) {
							alb.name = albumObj["name"_str].toString();
						}
						if (albumObj.contains("id"_str)) {
							alb.id = albumObj["id"_str].toString();
						}
						s.album = alb;
					}

					songsSection.results.push_back(s);
				}
			}

			result.songs = songsSection;
		}

		if (artistObj.contains("albums"_str) && artistObj["albums"_str].isObject()) {
			QJsonObject albumsObj = artistObj["albums"_str].toObject();
			Artist::Section<Artist::Album> albumsSection;

			parseSectionCommon(albumsObj, albumsSection);

			if (albumsObj.contains("results"_str) && albumsObj["results"_str].isArray()) {
				QJsonArray albumArray = albumsObj["results"_str].toArray();
				for (const QJsonValue& val : albumArray) {
					if (!val.isObject()) continue;
					QJsonObject albObj = val.toObject();

					Artist::Album a;
					// title
					if (albObj.contains("title"_str)) {
						a.title = albObj["title"_str].toString();
					}
					// thumbnails
					if (albObj.contains("thumbnails"_str) && albObj["thumbnails"_str].isArray()) {
						QJsonArray tArr = albObj["thumbnails"_str].toArray();
						a.thumbnails = parseThumbnails(tArr);
					}
					// browse_id
					if (albObj.contains("browseId"_str)) {
						a.browse_id = albObj["browseId"_str].toString();
					}
					// type
					if (albObj.contains("type"_str) && !albObj["type"_str].isNull()) {
						a.type = albObj["type"_str].toString();
					}

					albumsSection.results.push_back(a);
				}
			}
			result.albums = albumsSection;
		}

		if (artistObj.contains("singles"_str) && artistObj["singles"_str].isObject()) {
			QJsonObject singlesObj = artistObj["singles"_str].toObject();
			Artist::Section<Artist::Single> singlesSection;

			parseSectionCommon(singlesObj, singlesSection);

			if (singlesObj.contains("results"_str) && singlesObj["results"_str].isArray()) {
				QJsonArray singlesArray = singlesObj["results"_str].toArray();
				for (const QJsonValue& val : singlesArray) {
					if (!val.isObject()) continue;
					QJsonObject sObj = val.toObject();

					Artist::Single s;
					// title
					if (sObj.contains("title"_str)) {
						s.title = sObj["title"_str].toString();
					}
					// thumbnails
					if (sObj.contains("thumbnails"_str) && sObj["thumbnails"_str].isArray()) {
						QJsonArray stArr = sObj["thumbnails"_str].toArray();
						s.thumbnails = parseThumbnails(stArr);
					}
					// year
					if (sObj.contains("year"_str)) {
						s.year = sObj["year"_str].toString();
					}
					// browse_id
					if (sObj.contains("browse_id"_str)) {
						s.browse_id = sObj["browse_id"_str].toString();
					}

					singlesSection.results.push_back(s);
				}
			}
			result.singles = singlesSection;
		}

		if (artistObj.contains("videos"_str) && artistObj["videos"_str].isObject()) {
			QJsonObject videosObj = artistObj["videos"_str].toObject();
			Artist::Section<Artist::Video> videosSection;

			parseSectionCommon(videosObj, videosSection);

			if (videosObj.contains("results"_str) && videosObj["results"_str].isArray()) {
				QJsonArray videoArray = videosObj["results"_str].toArray();
				for (const QJsonValue& val : videoArray) {
					if (!val.isObject()) continue;
					QJsonObject vObj = val.toObject();

					Artist::Video v;
					// title
					if (vObj.contains("title"_str)) {
						v.title = vObj["title"_str].toString();
					}
					// thumbnails
					if (vObj.contains("thumbnails"_str) && vObj["thumbnails"_str].isArray()) {
						QJsonArray vtArr = vObj["thumbnails"_str].toArray();
						v.thumbnails = parseThumbnails(vtArr);
					}
					// views
					if (vObj.contains("views"_str) && !vObj["views"_str].isNull()) {
						v.views = vObj["views"_str].toString();
					}
					// video_id
					if (vObj.contains("videoId"_str)) {
						v.video_id = vObj["videoId"_str].toString();
					}
					// playlist_id
					if (vObj.contains("playlistId"_str)) {
						v.playlist_id = vObj["playlistId"_str].toString();
					}

					videosSection.results.push_back(v);
				}
			}
			result.videos = videosSection;
		}
		return result;
	}
}

YtMusicHttpService::YtMusicHttpService()
	: http_client_(BASE_URL) {
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

QCoro::Task<SongInfo> YtMusicHttpService::fetchSongInfo(const QString& video_id) {
	http_client_.setUrl(qFormat("%1/fetch_song").arg(BASE_URL));

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

QCoro::Task<std::optional<QList<LibraryPlaylist>>> YtMusicHttpService::fetchLibraryPlaylists() {
	http_client_.setUrl(qFormat("%1/fetch_library_playlists").arg(BASE_URL));
	http_client_.addAcceptJsonHeader();
	auto content = co_await http_client_.get();

	QList<LibraryPlaylist> results;
	QJsonDocument doc;
	if (!json_util::deserialize(content, doc)) {
		co_return std::nullopt;
	}

	const auto json_array = doc.array();

	// 遍歷 JSON 陣列中的物件
	for (const QJsonValue& value : json_array) {
		LibraryPlaylist playlist;
		QJsonObject playlist_obj = value.toObject();
		parsePlaylistJson(playlist, playlist_obj);
		results.push_back(playlist);
	}
	
	co_return MakeOptional<QList<LibraryPlaylist>>(std::move(results));
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

QCoro::Task<std::optional<album::Album>> YtMusicHttpService::fetchAlbum(const QString& browse_id) {
	http_client_.setUrl(qFormat("%1/get_album").arg(BASE_URL));

	QVariantMap map;
	map["browse_id"_str] = browse_id;
	http_client_.setJson(json_util::serialize(map));

	auto content = co_await http_client_.post();
	QJsonDocument doc;
	if (!json_util::deserialize(content, doc)) {
		co_return std::nullopt;
	}
	auto album = parseAlbum(doc.object());
	co_return MakeOptional<album::Album>(std::move(album));
}

QCoro::Task<std::optional<artist::Artist>> YtMusicHttpService::fetchArtist(const QString& channel_id) {
	http_client_.setUrl(qFormat("%1/fetch_artist").arg(BASE_URL));

	QVariantMap map;
	map["channel_id"_str] = channel_id;
	http_client_.setJson(json_util::serialize(map));

	QJsonDocument doc;
	auto content = co_await http_client_.post();
	if (!json_util::deserialize(content, doc)) {
		co_return std::nullopt;
	}

	auto artist = parseArtist(doc.object());
	co_return MakeOptional<artist::Artist>(std::move(artist));
}