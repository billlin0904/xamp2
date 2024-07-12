#include <widget/youtubedl/ytmusicservice.h>

#if 1
#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#define slots Q_SLOTS

#include <QBuffer>
#include <QImageReader>
#include <QPixmap>

#include <algorithm>
#include <widget/http.h>

namespace py = pybind11;
using namespace py::literals;

namespace {
    void dumpObject(LoggerPtr logger, const py::object& obj) {
        XAMP_LOG_D(logger, "{}", py::str(obj).cast<std::string>());
    }

    template <typename T>
    std::vector<T> extract_py_list(py::handle obj);

    template <typename T>
    std::optional<T> optional_key(py::handle obj, const char* name) {
        if (!obj.cast<py::dict>().contains(name)) {
            return std::nullopt;
        }

        return obj[name].cast<std::optional<T>>();
    }

    meta::Thumbnail extract_thumbnail(py::handle thumbnail) {
        return {
            thumbnail["url"].cast<std::string>(),
            thumbnail["width"].cast<int>(),
            thumbnail["height"].cast<int>()
        };
    }

    meta::Artist extract_meta_artist(py::handle artist) {
        return {
            artist["name"].cast<std::string>(),
            artist["id"].cast<std::optional<std::string>>()
        };
    };

    album::Track extract_album_track(py::handle track) {
        return {
            optional_key<bool>(track, "isExplicit"),
            track["title"].cast<std::string>(),
            extract_py_list<meta::Artist>(track["artists"]),
            track["album"].cast<std::optional<std::string>>(),
            track["videoId"].cast<std::optional<std::string>>(),  // E rated songs don't have a videoId
            track["duration"].cast<std::optional<std::string>>(), //
            track["likeStatus"].cast<std::optional<std::string>>()
        };
    }

    video_info::Format extract_format(py::handle format) {
        return {
            optional_key<float>(format, "quality"),
            format["url"].cast<std::string>(),
            format["vcodec"].cast<std::string>(),
            optional_key<std::string>(format, "acodec").value_or("none"), // returned inconsistently by yt-dlp
            optional_key<float>(format, "abr").value_or(0)
        };
    }

    meta::Album extract_meta_album(py::handle album) {
        return meta::Album{
            album["name"].cast<std::string>(),
            album["id"].cast<std::optional<std::string>>()
        };
    }

    watch::Playlist::Track extract_watch_track(py::handle track) {
        return {
            track["title"].cast<std::string>(),
            track["length"].cast<std::optional<std::string>>(),
            track["videoId"].cast<std::string>(),
            optional_key<std::string>(track, "playlistId"),
            extract_py_list<meta::Thumbnail>(track["thumbnail"]),
            track["likeStatus"].cast<std::optional<std::string>>(),
            extract_py_list<meta::Artist>(track["artists"]),
            [&]() -> std::optional<meta::Album> {
                if (!track.cast<py::dict>().contains("album")) {
                    return std::nullopt;
                }

                if (track["album"].is_none()) {
                    return std::nullopt;
                }

                return extract_meta_album(track["album"]);
            }()
        };
    }

    playlist::Track extract_playlist_track(py::handle track) {
        return {
            track["videoId"].cast<std::optional<std::string>>(),
            track["title"].cast<std::string>(),
            extract_py_list<meta::Artist>(track["artists"]),
            [&]() -> std::optional<meta::Album> {
                if (track["album"].is_none()) {
                    return std::nullopt;
                }
                return extract_meta_album(track["album"]);
            }(),
            [&]() -> std::optional<std::string> {
                if (!track.contains("duration")) {
                    return std::nullopt;
                }
                return optional_key<std::string>(track, "duration");
            }(),
            track["likeStatus"].cast<std::optional<std::string>>(),
            extract_py_list<meta::Thumbnail>(track["thumbnails"]),
            track["isAvailable"].cast<bool>(),
            optional_key<bool>(track, "isExplicit"),
            [&]() -> std::optional<std::string> {
                if (!track.contains("setVideoId")) {
                    return std::nullopt;
                }
                return track["setVideoId"].cast<std::optional<std::string>>();
                }()
        };
    }

    artist::Artist::Song::Album extract_song_album(py::handle album) {
        return {
            album["name"].cast<std::string>(),
            album["id"].cast<std::string>()
        };
    };

    template <typename T>
    auto extract_artist_section_results(py::handle section) {
        if (!section.cast<py::dict>().contains("results")) {
            return std::vector<T>();
        }

        const py::list py_results = section["results"];
        std::vector<T> results;
        std::transform(py_results.begin(), py_results.end(), std::back_inserter(results), [](py::handle result) {            
            if constexpr (std::is_same_v<T, artist::Artist::Song>) {
                return artist::Artist::Song{
                    result["videoId"].cast<std::string>(),
                    result["title"].cast<std::string>(),
                    extract_py_list<meta::Thumbnail>(result["thumbnails"]),
                    extract_py_list<meta::Artist>(result["artists"]),
                    extract_song_album(result["album"])
                };
            }
            else if constexpr (std::is_same_v<T, artist::Artist::Album>) {
                return artist::Artist::Album{
                    result["title"].cast<std::string>(),
                    extract_py_list<meta::Thumbnail>(result["thumbnails"]),
                    result["year"].cast<std::optional<std::string>>(),
                    result["browseId"].cast<std::string>(),
                    std::nullopt
                };
            }
            else if constexpr (std::is_same_v<T, artist::Artist::Single>) {
                return artist::Artist::Single{
                    result["title"].cast<std::string>(),
                    extract_py_list<meta::Thumbnail>(result["thumbnails"]),
                    result["year"].cast<std::string>(),
                    result["browseId"].cast<std::string>()
                };
            }
            else if constexpr (std::is_same_v<T, artist::Artist::Video>) {
                return artist::Artist::Video{
                    result["title"].cast<std::string>(),
                    extract_py_list<meta::Thumbnail>(result["thumbnails"]),
                    optional_key<std::string>(result, "views"),
                    result["videoId"].cast<std::string>(),
                    result["playlistId"].cast<std::string>()
                };
            }
            else {
                Py_UNREACHABLE();
            }
            });

        return results;
    }

    template <typename T, typename OP>
    std::optional<std::invoke_result_t<OP, T>> mapOptional(const std::optional<T>& optional, OP op) {
        if (optional.has_value()) {
            if constexpr (std::is_member_function_pointer_v<OP>) {
                return (&optional.value()->*op)();
            }
            else {
                return op(optional.value());
            }
        }

        return std::nullopt;
    }

    template<typename T>
    std::optional<artist::Artist::Section<T>> extract_artist_section(py::handle artist, const char* name) {
        if (artist.cast<py::dict>().contains(name)) {
            const auto section = artist[name];
            return artist::Artist::Section<T> {
                section["browseId"].cast<std::optional<std::string>>(),
                    extract_artist_section_results<T>(section),
                    optional_key<std::string>(section, "params")
            };
        }
        else {
            return std::nullopt;
        }
    }

    template <typename T>
    std::vector<T> extract_py_list(py::handle obj) {
        if (obj.is_none()) {
            return std::vector<T>();
        }

        const auto list = obj.cast<py::list>();
        std::vector<T> output;

        std::transform(list.begin(), list.end(), std::back_inserter(output), [](py::handle item) {
            if constexpr (std::is_same_v<T, meta::Thumbnail>) {
                return extract_thumbnail(item);
            }
            else if constexpr (std::is_same_v<T, meta::Artist>) {
                return extract_meta_artist(item);
            }
            else if constexpr (std::is_same_v<T, album::Track>) {
                return extract_album_track(item);
            }
            else if constexpr (std::is_same_v<T, playlist::Track>) {
                return extract_playlist_track(item);
            }
            else if constexpr (std::is_same_v<T, video_info::Format>) {
                return extract_format(item);
            }
            else if constexpr (std::is_same_v<T, watch::Playlist::Track>) {
                return extract_watch_track(item);
            }
            else {
                return item.cast<T>();
            }
            });

        return output;
    }
}

XAMP_DECLARE_LOG_NAME(YtMusicService);
XAMP_DECLARE_LOG_NAME(YtMusicInterop);

class YtMusicInterop::YtMusicInteropImpl {
public:    
    std::optional<std::string> auth;
    std::optional<std::string> user;
    std::optional<bool> requests_session;
    std::optional<std::map<std::string, std::string> > proxies;
    std::string language;
    std::string location;
    LoggerPtr logger;

    std::optional<search::SearchResultItem> extract_search_result(py::handle result) {
        const auto resultType = result["resultType"].cast<std::string>();

        if (result["category"].cast<std::optional<std::string>>() == "Top result") {
            return search::TopResult{
                result["category"].cast<std::string>(),
                result["resultType"].cast<std::string>(),
                optional_key<std::string>(result, "videoId"),
                optional_key<std::string>(result, "title"),
                extract_py_list<meta::Artist>(result["artists"]),
                extract_py_list<meta::Thumbnail>(result["thumbnails"])
            };
        }

        if (resultType == "video") {
            return search::Video{
                {
                    result["videoId"].cast<std::string>(),
                    result["title"].cast<std::string>(),
                    extract_py_list<meta::Artist>(result["artists"]),
                    optional_key<std::string>(result, "duration"),
                    extract_py_list<meta::Thumbnail>(result["thumbnails"])
                },
                optional_key<std::string>(result, "views")
            };
        }
        else if (resultType == "song") {
            return search::Song{
                {
                    result["videoId"].cast<std::string>(),
                    result["title"].cast<std::string>(),
                    extract_py_list<meta::Artist>(result["artists"]),
                    optional_key<std::string>(result, "duration"),
                    extract_py_list<meta::Thumbnail>(result["thumbnails"])
                },
                result["album"].is_none() ? std::optional<meta::Album> {} : extract_meta_album(result["album"]),
                optional_key<bool>(result, "isExplicit")
            };
        }
        else if (resultType == "album") {
            return search::Album{
                result["browseId"].cast<std::optional<std::string>>(),
                result["title"].cast<std::string>(),
                result["type"].cast<std::string>(),
                extract_py_list<meta::Artist>(result["artists"]),
                result["year"].cast<std::optional<std::string>>(),
                result["isExplicit"].cast<bool>(),
                extract_py_list<meta::Thumbnail>(result["thumbnails"])
            };
        }
        else if (resultType == "playlist") {
            return search::Playlist{
                result["browseId"].cast<std::string>(),
                result["title"].cast<std::string>(),
                result["author"].cast<std::optional<std::string>>(),
                result["itemCount"].cast<std::string>(),
                extract_py_list<meta::Thumbnail>(result["thumbnails"])
            };
        }
        else if (resultType == "artist") {
            return search::Artist{
                result["browseId"].cast<std::string>(),
                result["artist"].cast<std::string>(),
                optional_key<std::string>(result, "shuffleId"),
                optional_key<std::string>(result, "radioId"),
                extract_py_list<meta::Thumbnail>(result["thumbnails"])
            };
        }
        else if (resultType == "profile") {
            return search::Profile{
                result["name"].cast<std::string>()
            };
        }
        else {
            XAMP_LOG_W(logger, "Warning: Unsupported search result type found, It's called: {}", resultType);
            return std::nullopt;
        }
    }

    YtMusicInteropImpl(const std::optional<std::string>& auth,
        const std::optional<std::string>& user,
        const std::optional<bool> requests_session,
        const std::optional<std::map<std::string, std::string>>& proxies,
        const std::string& language,
        const std::string& location)
	        : auth(auth)
			, user(user)
			, requests_session(requests_session)
			, proxies(proxies)
			, language(language)
			, location(location) {
        logger = XampLoggerFactory.GetLogger(kYtMusicInteropLoggerName);
        ytmusic_ = py::none();
        ytdl_ = py::none();
    }

    ~YtMusicInteropImpl() = default;

    py::object& get_ytmusic() {
        if (ytmusic_.is_none()) {
            ytmusicapi_module_ = py::module::import("ytmusicapi");
            ytmusic_ = ytmusicapi_module_.attr("YTMusic")(auth, user, requests_session, proxies, language, location);
        }
        return ytmusic_;
    }

    py::module& get_ytmusicapi() {
        return ytmusicapi_module_;
    }

    py::object& get_ytdl() {
        if (ytdl_.is_none()) {
            py::dict ydl_opts;
        	ydl_opts["cookiesfrombrowser"] = create_cookies_from_browser("firefox");
            ytdl_ = py::module::import("yt_dlp").attr("YoutubeDL")(ydl_opts);
        }
        return ytdl_;
    }

private:
    static py::tuple create_cookies_from_browser(const std::string& browser_name,
        const std::optional<std::string>& profile_name = std::nullopt,
        const std::optional<std::string>& keyring_name = std::nullopt,
        const std::optional<std::string>& container_name = std::nullopt) {
        return py::make_tuple(browser_name, profile_name, keyring_name, container_name);
    }

    py::module ytmusicapi_module_;
    py::object ytmusic_;
    py::object ytdl_;
};

YtMusicService::YtMusicService(QObject* parent)
	: BaseService(parent) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(YtMusicInterop));
}

void YtMusicService::cancelRequested() {
    is_stop_ = true;
}

QFuture<bool> YtMusicService::initialAsync() {
    return invokeAsync([this]() {
        py::gil_scoped_acquire guard{};
        interop()->initial();
        return true;
        }, InvokeType::INVOKE_IMMEDIATELY);
}

QFuture<bool> YtMusicService::cleanupAsync() {
    return invokeAsync([this]() {        
        py::gil_scoped_acquire guard{};
    	interop_.reset();
        return true;
        }, InvokeType::INVOKE_IMMEDIATELY);
}

QFuture<std::vector<std::string>> YtMusicService::searchSuggestionsAsync(const QString& query, bool detailed_runs) {
    return invokeAsync([this, query, detailed_runs]() {
        py::gil_scoped_acquire guard{};
        return interop()->searchSuggestions(query.toStdString(), detailed_runs);
        });
}

QFuture<std::vector<search::SearchResultItem>> YtMusicService::searchAsync(const QString& query,
    const std::optional<std::string>& filter) {
    return invokeAsync([this, query, filter]() {
        py::gil_scoped_acquire guard{};
        return interop()->search(query.toStdString(), filter);
        });
}

QFuture<video_info::VideoInfo> YtMusicService::extractVideoInfoAsync(const QString& video_id) {
    return invokeAsync([this, video_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->extractInfo(video_id.toStdString());
        });
}

QFuture<int32_t> YtMusicService::downloadAsync(const QString& url) {
    return invokeAsync([this, url]() {
        py::gil_scoped_acquire guard{};
        return interop()->download(url.toStdString());
        });
}

QFuture<std::string> YtMusicService::createPlaylistAsync(const QString& title,
    const QString& description,
    PrivateStatus status,
    const std::vector<std::string>& video_ids,
    const std::optional<std::string>& source_playlist) {
    return invokeAsync([this, title, description, status, video_ids, source_playlist]() {
        return interop()->createPlaylistAsync(title.toStdString(), description.toStdString(), status, video_ids, source_playlist);
        });
}

QFuture<bool> YtMusicService::editPlaylistAsync(const QString& playlist_id,
    const QString& title,
    const QString& description,
	PrivateStatus status,
    const std::optional<std::tuple<std::string, std::string>>& move_item,
	const std::optional<std::string>& add_playlist_id, 
    const std::optional<std::string>& add_to_top) {
    return invokeAsync([this, playlist_id, title, description, status, move_item, add_playlist_id, add_to_top]() {
        py::gil_scoped_acquire guard{};
        return interop()->editPlaylist(playlist_id.toStdString(),
			title.toStdString(), description.toStdString(), status, move_item, add_playlist_id, add_to_top);
        });
}

QFuture<edit::PlaylistEditResults> YtMusicService::addPlaylistItemsAsync(const QString& playlist_id, const std::vector<std::string> &video_ids, const std::optional<std::string>& source_playlist, bool duplicates) {
    return invokeAsync([this, playlist_id, video_ids, source_playlist, duplicates]() {
        py::gil_scoped_acquire guard{};
        return interop()->addPlaylistItems(playlist_id.toStdString(), video_ids, source_playlist, duplicates);
        });
}

QFuture<bool> YtMusicService::removePlaylistItemsAsync(const QString& playlist_id, const std::vector<edit::PlaylistEditResultData>& videos) {
    return invokeAsync([this, playlist_id, videos]() {
        py::gil_scoped_acquire guard{};
        return interop()->removePlaylistItems(playlist_id.toStdString(), videos);
        });
}

QFuture<bool> YtMusicService::deletePlaylistAsync(const QString& playlist_id) {    
    return invokeAsync([this, playlist_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->deletePlaylist(playlist_id.toStdString());
        });
}

QFuture<bool> YtMusicService::rateSongAsync(const QString& video_id, SongRating rating) {
    return invokeAsync([this, video_id, rating]() {
        py::gil_scoped_acquire guard{};
        return interop()->rateSong(video_id.toStdString(), rating);
        });
}

QFuture<watch::Playlist> YtMusicService::fetchWatchPlaylistAsync(const std::optional<QString>& video_id, 
    const std::optional<QString>& playlist_id) {
    return invokeAsync([this, video_id, playlist_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->getWatchPlaylist(
            mapOptional(video_id, &QString::toStdString),
            mapOptional(playlist_id, &QString::toStdString)
        );
    });
}

QFuture<Lyrics> YtMusicService::fetchLyricsAsync(const QString& browse_id) {
    return invokeAsync([this, browse_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->getLyrics(browse_id.toStdString());
        });
}

QFuture<artist::Artist> YtMusicService::fetchArtistAsync(const QString& channel_id) {
    return invokeAsync([this, channel_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->getArtist(channel_id.toStdString());
        });
}

QFuture<album::Album> YtMusicService::fetchAlbumAsync(const QString& browse_id) {
    return invokeAsync([this, browse_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->getAlbum(browse_id.toStdString());
        });
}

QFuture<std::optional<song::Song>> YtMusicService::fetchSongAsync(const QString& video_id) {
    return invokeAsync([this, video_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->getSong(video_id.toStdString());
        });
}

QFuture<playlist::Playlist> YtMusicService::fetchPlaylistAsync(const QString& playlist_id) {
    return invokeAsync([this, playlist_id]() {
        py::gil_scoped_acquire guard{};
        return interop()->getPlaylist(playlist_id.toStdString());
        });
}

QFuture<std::vector<library::Playlist>> YtMusicService::fetchLibraryPlaylistAsync() {
    return invokeAsync([this]() {
        py::gil_scoped_acquire guard{};
        return interop()->getLibraryPlaylists();
        });
}

QFuture<std::vector<artist::Artist::Album>> YtMusicService::fetchArtistAlbumsAsync(const QString& channel_id, 
    const QString& params) {
    return invokeAsync([this, channel_id, params]() {
        py::gil_scoped_acquire guard{};
        return interop()->getArtistAlbums(channel_id.toStdString(), params.toStdString());
        });
}

YtMusicInterop* YtMusicService::interop() {
    return interop_.get();
}

YtMusicInterop::YtMusicInterop(const std::optional<std::string>& auth,
    const std::optional<std::string>& user,
    const std::optional<bool> requests_session,
    const std::optional<std::map<std::string, std::string>>& proxies,
    const std::string& language,
    const std::string& location)
    : impl_(MakeAlign<YtMusicInteropImpl>(auth, user, requests_session, proxies, language, location)) {
}

XAMP_PIMPL_IMPL(YtMusicInterop)

void YtMusicInterop::initial() {
    impl_->get_ytdl();
    impl_->get_ytmusic();    
}

std::vector<std::string> YtMusicInterop::searchSuggestions(const std::string& query, bool detailed_runs) const {
    const auto suggestions = impl_->get_ytmusic().attr("get_search_suggestions")(query, detailed_runs);
    dumpObject(impl_->logger, suggestions);
    return extract_py_list<std::string>(suggestions);
}

artist::Artist YtMusicInterop::getArtist(const std::string& channel_id) const {
    const auto artist = impl_->get_ytmusic().attr("get_artist")(channel_id);
    return artist::Artist{
        optional_key<std::string>(artist, "description"),
        artist["views"].cast<std::optional<std::string>>(),
        artist["name"].cast<std::string>(),
        artist["channelId"].cast<std::string>(),
        artist["subscribers"].cast<std::optional<std::string>>(),
        artist["subscribed"].cast<bool>(),
        extract_py_list<meta::Thumbnail>(artist["thumbnails"]),
        extract_artist_section<artist::Artist::Song>(artist, "songs"),
        extract_artist_section<artist::Artist::Album>(artist, "albums"),
        extract_artist_section<artist::Artist::Single>(artist, "singles"),
        extract_artist_section<artist::Artist::Video>(artist, "videos"),
    };
}

album::Album YtMusicInterop::getAlbum(const std::string& browse_id) const {
    const auto album = impl_->get_ytmusic().attr("get_album")(browse_id);
    return {
        album["title"].cast<std::string>(),
        album["trackCount"].cast<int>(),
        [&]() -> std::string {
            if (!album.contains("duration")) {
                return {};
            }
            return album["duration"].cast<std::string>();
        }(),
        album["audioPlaylistId"].cast<std::string>(),
        optional_key<std::string>(album, "year"),
        optional_key<std::string>(album, "description"),
        extract_py_list<meta::Thumbnail>(album["thumbnails"]),
        extract_py_list<album::Track>(album["tracks"]),
        extract_py_list<meta::Artist>(album["artists"])
    };
}

std::optional<song::Song> YtMusicInterop::getSong(const std::string& video_id) const {
    const auto song = impl_->get_ytmusic().attr("get_song")(video_id);
    const auto video_details = song["videoDetails"].cast<py::dict>();

    if (!video_details.contains("videoId")) {
        return std::nullopt;
    }

    return song::Song{
        video_details["videoId"].cast<std::string>(),
        video_details["title"].cast<std::string>(),
        video_details["lengthSeconds"].cast<std::string>(),
        video_details["channelId"].cast<std::string>(),
        video_details["isOwnerViewing"].cast<bool>(),
        video_details["isCrawlable"].cast<bool>(),
        song::Song::Thumbnail {
            extract_py_list<meta::Thumbnail>(video_details["thumbnail"]["thumbnails"])
        },
        video_details["viewCount"].cast<std::string>(),
        video_details["author"].cast<std::string>(),
        video_details["isPrivate"].cast<bool>(),
        video_details["isUnpluggedCorpus"].cast<bool>(),
        video_details["isLiveContent"].cast<bool>(),
        [&]() -> std::vector<std::string> {
            if (video_details.contains("artists")) {
                return extract_py_list<std::string>(video_details["artists"]);
            }
		else {
				return {};
			}
		}(),
    };
}

playlist::Playlist YtMusicInterop::getPlaylist(const std::string& playlist_id, int limit) const {
    const auto playlist = impl_->get_ytmusic().attr("get_playlist")(playlist_id, limit);
    auto extract_author = [&]() -> meta::Artist {
        if (playlist.contains("author")) {
            return extract_meta_artist(playlist["author"]);
        }
        else {
            return {};
        }
    };

    dumpObject(impl_->logger, playlist);

    auto extract_duration = [&]() -> std::string {
        if (playlist["duration"].is_none()) {
            return "";
        }
        return playlist["duration"].cast<std::string>();
    };

    return {
        playlist["id"].cast<std::string>(),
        playlist["privacy"].cast<std::string>(),
        playlist["title"].cast<std::string>(),
        [&]() -> std::vector<meta::Thumbnail> {
        if (playlist.contains("thumbnails")) {
            return extract_py_list<meta::Thumbnail>(playlist["thumbnails"]);
        }
        else {
            return {};
        }
        }(),
        extract_author(),
        optional_key<std::string>(playlist, "year"),
        extract_duration(),
        playlist["trackCount"].cast<int>(),
        extract_py_list<playlist::Track>(playlist["tracks"]),
    };
}

std::vector<library::Playlist> YtMusicInterop::getLibraryPlaylists(int limit) const {
    const auto py_playlists = impl_->get_ytmusic().attr("get_library_playlists")(limit);
    std::vector<library::Playlist> result;

    std::transform(py_playlists.begin(),
                   py_playlists.end(), std::back_inserter(result), [](py::handle playlist) {
        return library::Playlist {
        playlist["playlistId"].cast<std::string>(),
        playlist["title"].cast<std::string>(),
        extract_py_list<meta::Thumbnail>(playlist["thumbnails"])
        };
    });

    return result;
}

std::vector<artist::Artist::Album> YtMusicInterop::getArtistAlbums(const std::string& channel_id, const std::string& params) const {
    const auto py_albums = impl_->get_ytmusic().attr("get_artist_albums")(channel_id, params);
    std::vector<artist::Artist::Album> albums;

    std::transform(py_albums.begin(), py_albums.end(), std::back_inserter(albums), [](py::handle album) {
        return artist::Artist::Album{
            album["title"].cast<std::string>(),
            extract_py_list<meta::Thumbnail>(album["thumbnails"]),
            album["year"].cast<std::string>(),
            album["browseId"].cast<std::string>(),
            album["type"].cast<std::string>()
        };
        });

    return albums;
}

int32_t YtMusicInterop::download(const std::string& url) const {
    return impl_->get_ytdl().attr("download")(url).cast<int32_t>();
}

video_info::VideoInfo YtMusicInterop::extractInfo(const std::string& video_id) const {
    if (video_id.empty()) {
        return {};
    }
    const auto info = impl_->get_ytdl().attr("extract_info")(video_id, "download"_a = py::bool_(false));
    dumpObject(impl_->logger, info);
    return {
        info["id"].cast<std::string>(),
        info["title"].cast<std::string>(),
        info.contains("artist") ? info["artist"].cast<std::string>() : "",
        info.contains("channel") ? info["channel"].cast<std::string>() : "",
        extract_py_list<video_info::Format>(info["formats"]),
        info["thumbnail"].cast<std::string>()
    };
}

bool YtMusicInterop::rateSong(const std::string& video_id, SongRating rating) const {
    if (video_id.empty()) {
        return false;
    }
    const auto result = impl_->get_ytmusic().attr("rate_song")(video_id, EnumToString(rating));
    dumpObject(impl_->logger, result);
    return true;
}

std::string YtMusicInterop::createPlaylistAsync(const std::string& title,
                                                const std::string& description,
                                                PrivateStatus status,
                                                const std::vector<std::string>& video_ids,
                                                const std::optional<std::string>& source_playlist) const {
    if (title.empty()) {
        return {};
    }
    const auto result = impl_->get_ytmusic().attr("create_playlist")(title,        
        description, 
        EnumToString(status),
        py::cast(video_ids), 
        source_playlist);
    dumpObject(impl_->logger, result);
    return result.cast<std::string>();
}

bool YtMusicInterop::editPlaylist(const std::string& playlist_id,
    const std::string& title,
    const std::string& description,
    PrivateStatus status,
    const std::optional<std::tuple<std::string, std::string>>& move_item,
    const std::optional<std::string>& add_playlist_id,
	const std::optional<std::string>& add_to_top) const {
    if (playlist_id.empty()) {
        return false;
    }
    const auto result = impl_->get_ytmusic().attr("edit_playlist")(playlist_id,
        title,
        description, 
        EnumToString(status), 
        move_item, 
        add_playlist_id,
        add_to_top);
    dumpObject(impl_->logger, result);
    return true;
}

edit::PlaylistEditResults YtMusicInterop::addPlaylistItems(const std::string& playlist_id,
                                                           const std::vector<std::string>& video_ids,
                                                           const std::optional<std::string>& source_playlist, 
                                                           bool duplicates) const {
    if (playlist_id.empty()) {
        return {};
    }
    const auto result = impl_->get_ytmusic().attr("add_playlist_items")(playlist_id,        
        py::cast(video_ids),
        source_playlist,
        duplicates);
    dumpObject(impl_->logger, result);
    std::vector<edit::PlaylistEditResultData> playlist_results;
    for (const auto& item : result["playlistEditResults"]) {
	    edit::PlaylistEditResultData result_data;
        result_data.videoId = item["videoId"].cast<std::string>();
        result_data.setVideoId = item["setVideoId"].cast<std::string>();
        result_data.multi_select_data.multi_select_params = item["multiSelectData"]["multiSelectParams"].cast<std::string>();
        result_data.multi_select_data.multi_select_item = item["multiSelectData"]["multiSelectItem"].cast<std::string>();
        playlist_results.push_back(result_data);
    }
    return {
        result["status"].cast<std::string>(),
        playlist_results,
    };
}

bool YtMusicInterop::removePlaylistItems(const std::string& playlist_id,
	const std::vector<edit::PlaylistEditResultData>& video_ids) const {
    if (playlist_id.empty()) {
        return false;
    }

    py::list py_list;
    for (const auto& item : video_ids) {
        py::dict py_dict;
        py_dict["videoId"] = item.videoId;
        py_dict["setVideoId"] = item.setVideoId;
        py_list.append(py_dict);
    }

    const auto result = impl_->get_ytmusic().attr("remove_playlist_items")(playlist_id, py_list);
    dumpObject(impl_->logger, result);
    return true;
}

bool YtMusicInterop::deletePlaylist(const std::string& playlist_id) const {
    const auto result = impl_->get_ytmusic().attr("delete_playlist")(playlist_id);
    dumpObject(impl_->logger, result);
    return true;
}

watch::Playlist YtMusicInterop::getWatchPlaylist(const std::optional<std::string>& video_id,
                                                 const std::optional<std::string>& playlist_id,
                                                 int limit) const {
    const auto playlist = impl_->get_ytmusic().attr("get_watch_playlist")("videoId"_a = video_id,
        "playlistId"_a = playlist_id,
        "limit"_a = py::int_(limit));
    return {
        extract_py_list<watch::Playlist::Track>(playlist["tracks"]),
        playlist["lyrics"].cast<std::optional<std::string>>()
    };
}


Lyrics YtMusicInterop::getLyrics(const std::string& browse_id) const {
	const auto lyrics = impl_->get_ytmusic().attr("get_lyrics")(browse_id);
    dumpObject(impl_->logger, lyrics);
    return {
        lyrics["source"].cast<std::optional<std::string>>(),
        lyrics["lyrics"].cast<std::optional<std::string>>()
    };
}

std::vector<search::SearchResultItem> YtMusicInterop::search(
    const std::string& query,
    const std::optional<std::string>& filter,
    const std::optional<std::string>& scope,
    const int limit,
    const bool ignore_spelling) const {
    std::vector<search::SearchResultItem> output;
    if (query.empty()) {
        return output;
    }

    const auto results = impl_->get_ytmusic().attr("search") (
       "query"_a = query,
       "filter"_a = filter,
       "scope"_a = scope,
       "limit"_a = limit,
       "ignore_spelling"_a = ignore_spelling
       ).cast<py::list>();

    for (const auto& result : results) {
        if (result.is_none()) {
            continue;
        }

        try {
            if (const auto opt = impl_->extract_search_result(result); opt.has_value()) {
                output.push_back(opt.value());
            }
        }
        catch (const std::exception& e) {
            XAMP_LOG_D(impl_->logger, "Failed to parse search result because:{}", e.what());
        }
    }
    
    return output;
}
#endif
