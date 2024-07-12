//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtConcurrent/QtConcurrent>
#include <QObject>
#include <QFuture>

#include <base/lazy_storage.h>
#include <widget/widget_shared.h>
#include <widget/util/str_util.h>
#include <base/enum.h>
#include <base/stl.h>
#include <widget/widget_shared_global.h>
#include <widget/baseservice.h>

namespace meta {
	struct Thumbnail {
		std::string url;
		int width;
		int height;

		bool operator<(const Thumbnail& other) const {
			return height < other.height;
		}
	};

	struct Artist {
		std::string name;
		std::optional<std::string> id;
	};

	struct Album {
		std::string name;
		std::optional<std::string> id;
	};
}

namespace search {
	struct Media {
		std::string video_id;
		std::string title;
		std::vector<meta::Artist> artists;
		std::optional<std::string> duration;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Video : Media {
		std::optional<std::string> views;
	};

	struct Playlist {
		std::string browse_id;
		std::string title;
		std::optional<std::string> author;
		std::string item_count;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Song : Media {
		std::optional<meta::Album> album;
		std::optional<bool> is_explicit;
	};

	struct Album {
		std::optional<std::string> browse_id;
		std::string title;
		std::string type;
		std::vector<meta::Artist> artists;
		std::optional<std::string> year;
		bool is_explicit;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Artist {
		std::string browse_id;
		std::string artist;
		std::optional<std::string> shuffle_id;
		std::optional<std::string> radio_id;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct TopResult {
		std::string category;
		std::string result_type;
		std::optional<std::string> video_id;
		std::optional<std::string> title;
		std::vector<meta::Artist> artists;
		std::vector<meta::Thumbnail> thumbnails;
	};

	struct Profile {
		std::string profile;
	};

	using SearchResultItem = std::variant<Video, Playlist, Song, Album, Artist, TopResult, Profile>;
};


namespace artist {
	struct Artist {
		template<typename T>
		struct Section {
			std::optional<std::string> browse_id;
			std::vector<T> results;
			std::optional<std::string> params;
		};

		struct Song {
			struct Album {
				std::string name;
				std::string id;
			};

			std::string video_id;
			std::string title;
			std::vector<meta::Thumbnail> thumbnails;
			std::vector<meta::Artist> artist;
			Album album;
		};

		struct Album {
			std::string title;
			std::vector<meta::Thumbnail> thumbnails;
			std::optional<std::string> year;
			std::string browse_id;
			std::optional<std::string> type;
		};

		struct Video {
			std::string title;
			std::vector<meta::Thumbnail> thumbnails;
			std::optional<std::string> views;
			std::string video_id;
			std::string playlist_id;
		};

		struct Single {
			std::string title;
			std::vector<meta::Thumbnail> thumbnails;
			std::string year;
			std::string browse_id;
		};

		std::optional<std::string> description;
		std::optional<std::string> views;
		std::string name;
		std::string channel_id;
		std::optional<std::string> subscribers;
		bool subscribed;
		std::vector<meta::Thumbnail> thumbnails;
		std::optional<Section<Song>> songs;
		std::optional<Section<Album>> albums;
		std::optional<Section<Single>> singles;
		std::optional<Section<Video>> videos;
	};
}

namespace album {
	struct Track {
		std::optional<bool> is_explicit;
		std::string title;
		std::vector<meta::Artist> artists;
		std::optional<std::string> album;
		std::optional<std::string> video_id;
		std::optional<std::string> duration;
		std::optional<std::string> like_status;
	};

	struct Album {
		struct ReleaseDate {
			int year;
			int month;
			int day;
		};

		std::string title;
		int track_count;
		std::string duration;
		std::string audio_playlist_id;
		std::optional<std::string> year;
		std::optional<std::string> description;
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

		std::string artist() {
			const char* const delim = ", ";

			std::ostringstream imploded;
			std::copy(artists.begin(), artists.end(),
				std::ostream_iterator<std::string>(imploded, delim));
			return imploded.str();
		}

		std::string video_id;
		std::string title;
		std::string length;
		std::string channel_id;
		bool is_owner_viewer;
		bool is_crawlable;
		Thumbnail thumbnail;
		std::string view_count;
		std::string author;
		bool is_private;
		bool is_unplugged_corpus;
		bool is_live_content;
		std::vector<std::string> artists;
	};
}

namespace playlist {
	struct Track {
		std::optional<std::string> video_id;
		std::string title;
		std::vector<meta::Artist> artists;
		std::optional<meta::Album> album;
		std::optional<std::string> duration;
		std::optional<std::string> like_status;
		std::vector<meta::Thumbnail> thumbnails;
		bool is_available;
		std::optional<bool> is_explicit;
		std::optional<std::string> set_video_id;
	};

	struct Playlist {
		std::string id;
		std::string privacy;
		std::string title;
		std::vector<meta::Thumbnail> thumbnails;
		meta::Artist author;
		std::optional<std::string> year;
		std::string duration;
		int track_count;
		std::vector<Track> tracks;
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
		return String::ToString(track.video_id.value() + " ");
	}
	return String::ToString(track.video_id.value() + " " + track.set_video_id.value());
}

/*
* First: video id
* Second: set video id
*/
inline std::pair<QString, QString> parseYtMusicPath(const QString& id) {
	auto parts = id.split(qTEXT(" "), Qt::SkipEmptyParts);
	if (parts.size() != 2) {
		return std::make_pair(parts[0], kEmptyString);
	}
	return std::make_pair(parts[0], parts[1]);
}

namespace video_info {
	struct Format {
		std::optional<float> quality;
		std::string url;
		std::string vcodec;
		std::string acodec;
		float abr;

		// More, but not interesting for us right now
		bool operator<(const Format& other) const {
			if (!quality) {
				return false;
			}
			if (acodec == "none") {
				return false;
			}
			return other.quality < quality;
		}
	};

	struct VideoInfo {
		std::string id;
		std::string title;
		std::string artist;
		std::string channel;
		std::vector<Format> formats;
		std::string thumbnail;

		// More, but not interesting for us right now
	};
}

namespace watch {
	struct Playlist {
		struct Track {
			std::string title;
			std::optional<std::string> length;
			std::string video_id;
			std::optional<std::string> playlistId;
			std::vector<meta::Thumbnail> thumbnail;
			std::optional<std::string> like_status;
			std::vector<meta::Artist> artists;
			std::optional<meta::Album> album;
		};

		std::vector<Track> tracks;
		std::optional<std::string> lyrics;
	};
}

namespace library {
	struct Playlist {
		std::string playlistId;
		std::string title;
		std::vector<meta::Thumbnail> thumbnail;
	};
}

namespace edit {
	struct MultiSelectData {
		std::string multi_select_params;
		std::string multi_select_item;
	};

	struct PlaylistEditResultData {
		std::optional<std::string> videoId;
		std::optional<std::string> setVideoId;
		MultiSelectData multi_select_data;
	};

	struct PlaylistEditResults {
		std::string status;
		std::vector<PlaylistEditResultData> result_data;
	};
}

struct Lyrics {
	std::optional<std::string> source;
	std::optional<std::string> lyrics;
};

class YtMusicInterop;

#if 1
class YtMusicInterop {
public:
	explicit YtMusicInterop(const std::optional<std::string>& auth = "oauth.json",
		const std::optional<std::string>& user = std::nullopt,
		const std::optional<bool> requests_session = true,
		const std::optional<std::map<std::string, std::string>>& proxies = std::nullopt,
		const std::string& language = "en",
		const std::string& location = "");

	XAMP_PIMPL(YtMusicInterop)

	void initial();

	XAMP_NO_DISCARD std::vector<std::string> searchSuggestions(const std::string& query, bool detailed_runs) const;

	XAMP_NO_DISCARD std::vector<search::SearchResultItem> search(const std::string& query,
		const std::optional<std::string>& filter = std::nullopt,
		const std::optional<std::string>& scope = std::nullopt,
		int limit = 10,
		bool ignore_spelling = false) const;

	XAMP_NO_DISCARD artist::Artist getArtist(const std::string& channel_id) const;

	XAMP_NO_DISCARD album::Album getAlbum(const std::string& browse_id) const;

	XAMP_NO_DISCARD std::optional<song::Song> getSong(const std::string& video_id) const;

	XAMP_NO_DISCARD std::vector<library::Playlist> getLibraryPlaylists(int limit = 25) const;

	XAMP_NO_DISCARD playlist::Playlist getPlaylist(const std::string& playlist_id, int limit = 1024) const;

	XAMP_NO_DISCARD std::vector<artist::Artist::Album> getArtistAlbums(const std::string& channel_id, const std::string& params) const;

	XAMP_NO_DISCARD watch::Playlist getWatchPlaylist(const std::optional<std::string>& video_id = std::nullopt,
		const std::optional<std::string>& playlist_id = std::nullopt,
		int limit = 25) const;

	XAMP_NO_DISCARD Lyrics getLyrics(const std::string& browse_id) const;

	XAMP_NO_DISCARD video_info::VideoInfo extractInfo(const std::string& video_id) const;

	XAMP_NO_DISCARD bool rateSong(const std::string& video_id, SongRating rating) const;

	XAMP_NO_DISCARD std::string createPlaylistAsync(const std::string& title,
		const std::string& description,
		PrivateStatus status,
		const std::vector<std::string>& video_ids,
		const std::optional<std::string>& source_playlist = std::nullopt) const;

	XAMP_NO_DISCARD bool editPlaylist(const std::string& playlist_id,
		const std::string& title,
		const std::string& description,
		PrivateStatus status,
		const std::optional<std::tuple<std::string, std::string>>& move_item = std::nullopt,
		const std::optional<std::string>& add_playlist_id = std::nullopt,
		const std::optional<std::string>& add_to_top = std::nullopt) const;

	XAMP_NO_DISCARD edit::PlaylistEditResults addPlaylistItems(const std::string& playlist_id,
		const std::vector<std::string>& video_ids,
		const std::optional<std::string>& source_playlist,
		bool duplicates) const;

	XAMP_NO_DISCARD bool removePlaylistItems(const std::string& playlist_id, 
		const std::vector<edit::PlaylistEditResultData>& videos) const;

	XAMP_NO_DISCARD bool deletePlaylist(const std::string& playlist_id) const;

	XAMP_NO_DISCARD int32_t download(const std::string& url) const;
private:
	class YtMusicInteropImpl;
	AlignPtr<YtMusicInteropImpl> impl_;
};

class XAMP_WIDGET_SHARED_EXPORT YtMusicService : public BaseService {	
public:
	explicit YtMusicService(QObject* parent = nullptr);

	QFuture<bool> initialAsync();

	QFuture<bool> cleanupAsync();

	QFuture<std::vector<std::string>> searchSuggestionsAsync(const QString& query, bool detailed_runs = false);

	QFuture<std::vector<search::SearchResultItem>> searchAsync(const QString& query, const std::optional<std::string>& filter);

	QFuture<artist::Artist> fetchArtistAsync(const QString& channel_id);

	QFuture<album::Album> fetchAlbumAsync(const QString& browse_id);

	QFuture<std::optional<song::Song>> fetchSongAsync(const QString& video_id);

	QFuture<playlist::Playlist> fetchPlaylistAsync(const QString& playlist_id);

	QFuture<std::vector<library::Playlist>> fetchLibraryPlaylistAsync();

	QFuture<std::vector<artist::Artist::Album>> fetchArtistAlbumsAsync(const QString& channel_id, const QString& params);

	QFuture<watch::Playlist> fetchWatchPlaylistAsync(const std::optional<QString>& video_id,
		const std::optional<QString>& playlist_id = std::nullopt);

	QFuture<Lyrics> fetchLyricsAsync(const QString& browse_id);

	QFuture<video_info::VideoInfo> extractVideoInfoAsync(const QString& video_id);

	QFuture<int32_t> downloadAsync(const QString& url);

	QFuture<std::string> createPlaylistAsync(const QString& title,
		const QString& description,
		PrivateStatus status,
		const std::vector<std::string>& video_ids,
		const std::optional<std::string>& source_playlist = std::nullopt);

	QFuture<bool> editPlaylistAsync(const QString& playlist_id,
		const QString& title,
		const QString& description,
		PrivateStatus status,
		const std::optional<std::tuple<std::string, std::string>>& move_item = std::nullopt,
		const std::optional<std::string>& add_playlist_id = std::nullopt,
		const std::optional<std::string>& add_to_top = std::nullopt);

	QFuture<edit::PlaylistEditResults> addPlaylistItemsAsync(const QString& playlist_id,
		const std::vector<std::string>& video_ids,
		const std::optional<std::string>& source_playlist = std::nullopt,
		bool duplicates = false);

	QFuture<bool> removePlaylistItemsAsync(const QString& playlist_id,
		const std::vector<edit::PlaylistEditResultData>& videos);

	QFuture<bool> deletePlaylistAsync(const QString& playlist_id);

	QFuture<bool> rateSongAsync(const QString& video_id, SongRating rating);

public slots:
	void cancelRequested();

private:
	YtMusicInterop* interop();	
	LocalStorage<YtMusicInterop> interop_;
};
#endif
