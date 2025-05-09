//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtConcurrent/QtConcurrent>
#include <QObject>
#include <QFuture>
#include <QCoroTask>

#include <base/lazy_storage.h>
#include <base/enum.h>
#include <base/stl.h>

#include <widget/httpx.h>
#include <widget/widget_shared.h>
#include <widget/util/str_util.h>
#include <widget/widget_shared_global.h>

namespace meta {
	struct Thumbnail {
		QString url;
		uint32_t width;
		uint32_t height;

		bool operator<(const Thumbnail& other) const {
			return height < other.height;
		}
	};

	struct Artist {
		QString name;
		std::optional<QString> id;
	};

	struct Album {
		QString name;
		std::optional<QString> id;
	};
}

namespace search {
	struct Media {
		QString video_id;
		QString title;
		std::vector<meta::Artist> artists;
		std::optional<QString> duration;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Video : Media {
		std::optional<QString> views;
	};

	struct Playlist {
		QString browse_id;
		QString title;
		std::optional<QString> author;
		QString item_count;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Song : Media {
		std::optional<meta::Album> album;
		std::optional<bool> is_explicit;
	};

	struct Album {
		std::optional<QString> browse_id;
		QString title;
		QString type;
		std::vector<meta::Artist> artists;
		std::optional<QString> year;
		bool is_explicit;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Artist {
		QString browse_id;
		QString artist;
		std::optional<QString> shuffle_id;
		std::optional<QString> radio_id;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct TopResult {
		QString category;
		QString result_type;
		std::optional<QString> video_id;
		std::optional<QString> title;
		std::vector<meta::Artist> artists;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Profile {
		QString profile;
	};

	using SearchResultItem = std::variant<Video, Playlist, Song, Album, Artist, TopResult, Profile>;
};


namespace artist {
	struct Artist {
		template<typename T>
		struct Section {
			std::optional<QString> browse_id;
			std::vector<T> results;
			std::optional<QString> params;
		};

		struct Song {
			struct Album {
				QString name;
				QString id;
			};

			QString video_id;
			QString title;
			std::vector<meta::Thumbnail> thumbnails;
			std::vector<meta::Artist> artist;
			Album album;
		};

		struct Album {
			QString title;
			std::vector<meta::Thumbnail> thumbnails;
			QString browse_id;
			QString type;
		};

		struct Video {
			QString title;
			std::vector<meta::Thumbnail> thumbnails;
			std::optional<QString> views;
			QString video_id;
			QString playlist_id;
		};

		struct Single {
			QString title;
			std::vector<meta::Thumbnail> thumbnails;
			QString year;
			QString browse_id;
		};

		bool subscribed;
		QString description;
		std::optional<QString> views;
		QString name;
		QString channel_id;
		std::optional<QString> subscribers;
		std::vector<meta::Thumbnail> thumbnails;
		std::optional<Section<Song>> songs;
		std::optional<Section<Album>> albums;
		std::optional<Section<Single>> singles;
		std::optional<Section<Video>> videos;
	};
}

namespace album {
	struct Track {
		uint32_t track = 0;
		QString title;
		std::optional<bool> is_explicit;
		std::vector<meta::Artist> artists;
		std::optional<QString> album;
		std::optional<QString> video_id;
		std::optional<QString> duration;
		std::optional<QString> like_status;
	};

	struct Album {
		struct ReleaseDate {
			uint32_t year;
			uint32_t month;
			uint32_t day;
		};

		QString title;
		uint32_t track_count;
		QString duration;
		QString audio_playlist_id;
		std::optional<QString> year;
		std::optional<QString> description;
		std::vector<meta::Thumbnail> thumbnails;
		std::vector<Track> tracks;
		std::vector<meta::Artist> artists;
	};
}

namespace song {
	struct Song {
		struct Thumbnail {
			std::vector<meta::Thumbnail> thumbnails;
		};

		QString artist() {
			const char* const delim = ", ";

			/*std::ostringstream imploded;
			std::copy(artists.begin(), artists.end(),
				std::ostream_iterator<std::string>(imploded, delim));
			return imploded.str();*/
			return kEmptyString;
		}

		bool is_owner_viewer;
		bool is_crawlable;
		bool is_private;
		bool is_unplugged_corpus;
		bool is_live_content;
		QString view_count;
		QString author;
		QString video_id;
		QString title;
		QString length;
		QString channel_id;
		Thumbnail thumbnail;
		std::vector<QString> artists;
	};
}

namespace playlist {
	struct Track {
		bool is_available;
		QString title;
		std::vector<meta::Artist> artists;
		std::optional<meta::Album> album;
		std::optional<QString> duration;
		std::optional<QString> like_status;
		std::vector<meta::Thumbnail> thumbnails;
		std::optional<bool> is_explicit;
		std::optional<QString> set_video_id;
		std::optional<QString> video_id;
	};

	struct Playlist {
		uint32_t track_count;
		QString id;
		QString privacy;
		QString title;
		QString duration;
		std::optional<QString> year;
		std::vector<Track> tracks;
		std::vector<meta::Thumbnail> thumbnails;
		meta::Artist author;
	};
}

XAMP_MAKE_ENUM(PrivateStatus,
	PUBLIC,
	PRIVATE,
	UNLISTED)

XAMP_MAKE_ENUM(SongRating,
	LIKE,
	DISLIKE,
	INDIFFERENT)

inline std::wstring makeYtMusicPath(const playlist::Track& track) {
	if (!track.set_video_id) {
		return QString(track.video_id.value() + " "_str).toStdWString();
	}
	return QString(track.video_id.value() + " "_str + track.set_video_id.value()).toStdWString();
}

inline std::wstring makeYtMusicPath(const artist::Artist::Song& track) {
	return QString(track.video_id + " "_str).toStdWString();
}

inline std::wstring makeYtMusicPath(const album::Track& track) {
	return QString(track.video_id.value() + " "_str).toStdWString();
}

/*
* First: video id
* Second: set video id
*/
inline std::pair<QString, QString> parseYtMusicPath(const QString& id) {
	auto parts = id.split(" "_str, Qt::SkipEmptyParts);
	if (parts.size() != 2) {
		return std::make_pair(parts[0], kEmptyString);
	}
	return std::make_pair(parts[0], parts[1]);
}

namespace video_info {
	struct Format {
		std::optional<float> quality;
		QString url;
		QString vcodec;
		QString acodec;
		float abr;

		// More, but not interesting for us right now
		bool operator<(const Format& other) const {
			if (!quality) {
				return false;
			}
			if (acodec == "none"_str) {
				return false;
			}
			return other.quality < quality;
		}
	};

	struct VideoInfo {
		QString id;
		QString title;
		QString artist;
		QString channel;
		std::vector<Format> formats;
		QString thumbnail;

		// More, but not interesting for us right now
	};
}

namespace watch {
	struct Playlist {
		struct Track {
			QString title;
			std::optional<QString> length;
			QString video_id;
			std::optional<QString> playlistId;
			std::vector<meta::Thumbnail> thumbnail;
			std::optional<QString> like_status;
			std::vector<meta::Artist> artists;
			std::optional<meta::Album> album;
		};

		std::vector<Track> tracks;
		std::optional<std::string> lyrics;
	};
}

namespace library {
	struct Playlist {
		QString playlistId;
		QString title;
		std::vector<meta::Thumbnail> thumbnail;
	};
}

namespace edit {
	struct MultiSelectData {
		QString multi_select_params;
		QString multi_select_item;
	};

	struct PlaylistEditResultData {
		std::optional<QString> videoId;
		std::optional<QString> setVideoId;
		MultiSelectData multi_select_data;
	};

	struct PlaylistEditResults {
		QString status;
		std::vector<PlaylistEditResultData> result_data;
	};
}

struct Lyrics {
	std::optional<std::string> source;
	std::optional<std::string> lyrics;
};

struct XAMP_WIDGET_SHARED_EXPORT LibraryPlaylist {
	QString title;
	QString playlist_id;
	std::vector<playlist::Track> tracks;
};

struct XAMP_WIDGET_SHARED_EXPORT SongInfo {
	QString download_url;
	QString thumbnail_base64;
	QString lyrics;
};

class XAMP_WIDGET_SHARED_EXPORT YtMusicHttpService {
public:
	YtMusicHttpService();

	QCoro::Task<SongInfo> fetchSongInfo(const QString& video_id);

	QCoro::Task<std::optional<QList<LibraryPlaylist>>> fetchLibraryPlaylists();

	QCoro::Task<LibraryPlaylist> fetchPlaylist(const QString& playlist_id);

	QCoro::Task<QString> createPlaylist(const QString& title,
		const QString& description = QString(),
		PrivateStatus status = PrivateStatus::PUBLIC,
		const QList<QString>& video_ids = QList<QString>(),
		const QString& source_playlist = QString());

	QCoro::Task<QString> editPlaylist(const QString& playlist_id, const QString& title);

	QCoro::Task<QString> deletePlaylist(const QString& playlist_id);

	QCoro::Task<QString> fetchLyrics(const QString& video_id);

	QCoro::Task<QList<QString>> searchSuggestions(const QString& query);

	QCoro::Task<QList<search::Album>> searchAlbum(const QString& query);

	QCoro::Task<std::optional<album::Album>> fetchAlbum(const QString& browse_id);

	QCoro::Task<std::optional<artist::Artist>> fetchArtist(const QString& channel_id);
private:
	http::HttpClient http_client_;
};