#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <QBuffer>
#include <QImageReader>
#include <QPixmap>

#include <algorithm>
#include <widget/http.h>
#include <widget/youtubedl/ytmusic.h>

namespace py = pybind11;
using namespace py::literals;

namespace {
    void dumps(py::handle obj) {
        //auto json = py::module::import("json");
        //py::print(json.attr("dumps")(obj, "indent"_a = py::int_(4)));
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
            optional_key<std::string>(format, "acodec").value_or("none") // returned inconsistently by yt-dlp
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
            optional_key<std::string>(track, "duration"),
            track["likeStatus"].cast<std::optional<std::string>>(),
            extract_py_list<meta::Thumbnail>(track["thumbnails"]),
            track["isAvailable"].cast<bool>(),
            optional_key<bool>(track, "isExplicit")
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
            dumps(item);

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

XAMP_DECLARE_LOG_NAME(YtMusic);
XAMP_DECLARE_LOG_NAME(YtMusicInterop);

class YtMusicInterop::YtMusicInteropImpl {
public:
    py::scoped_interpreter guard{};
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
        logger = LoggerManager::GetInstance().GetLogger(kYtMusicInteropLoggerName);
        ytmusic_ = py::none();
        ytdl_ = py::none();
    }

    py::object get_ytmusic() {
        if (ytmusic_.is_none()) {
            ytmusicapi_module = py::module::import("ytmusicapi");
            ytmusic_ = ytmusicapi_module.attr("YTMusic")(auth, user, requests_session, proxies, language, location);

            auto old_version = ytmusicapi_module.attr("__dict__").contains("_version");
            if (old_version) {
                XAMP_LOG_E(logger, "Running with outdated and untested version of ytmusicapi.");
            }
            else {
                const auto version = ytmusicapi_module.attr("__version__").cast<std::string>();
                XAMP_LOG_D(logger, "Running with untested version of ytmusicapi {}", version);
            }
        }
        return ytmusic_;
    }

    py::object get_ytdl() {
        if (ytdl_.is_none()) {
            ytdl_ = py::module::import("yt_dlp").attr("YoutubeDL")(py::dict());
        }
        return ytdl_;
    }

    py::module ytmusicapi_module;
private:
    py::object ytmusic_;
    py::object ytdl_;
};

YtMusic::YtMusic(QObject* parent)
	: QObject(parent) {
    logger_ = LoggerManager::GetInstance().GetLogger(kYtMusicInteropLoggerName);
}

void YtMusic::initial() {
    try {
        interop()->initial();
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }    
}

void YtMusic::search(const QString& query) {
    if (query.isEmpty()) {
        return;
    }
    try {
        emit searchCompleted(interop()->search(query.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }
}

void YtMusic::fetchArtist(const QString& channel_id) {    
    try {
        emit fetchArtistCompleted(interop()->getArtist(channel_id.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }
}

void YtMusic::fetchAlbum(const QString& browse_id) {    
    try {
        emit fetchAlbumCompleted(interop()->getAlbum(browse_id.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }
}

void YtMusic::fetchSong(const QString& video_id) {
    try {
        if (video_id.isEmpty()) {
            return;
        }
        emit fetchSongCompleted(interop()->getSong(video_id.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }    
}

void YtMusic::fetchPlaylist(const QString& playlist_id) {
    try {
        emit fetchPlaylistCompleted(interop()->getPlaylist(playlist_id.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }    
}

void YtMusic::fetchArtistAlbums(const QString& channel_id, const QString& params) {
    try {
        emit fetchArtistAlbumsCompleted(interop()->getArtistAlbums(channel_id.toStdString(), params.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }    
}

void YtMusic::extractVideoInfo(const std::any& context, const QString& video_id) {    
    try {
        emit extractVideoInfoCompleted(context, interop()->extractInfo(video_id.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }
}

void YtMusic::fetchWatchPlaylist(const std::optional<QString>& video_id,
    const std::optional<QString>& playlist_id) {
    try {
        emit fetchWatchPlaylistCompleted(interop()->getWatchPlaylist(
            mapOptional(video_id, &QString::toStdString),
            mapOptional(playlist_id, &QString::toStdString)
        ));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }    
}

void YtMusic::fetchLyrics(const QString& browse_id) {
    try {
        emit fetchLyricsCompleted(interop()->getLyrics(browse_id.toStdString()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_E(logger_, "{}", e.what());
    }    
}

void YtMusic::cleanup() {
    interop_.reset();
    emit cleanupCompleted();
}

void YtMusic::fetchThumbnail(int32_t id, const video_info::VideoInfo& video_info) {
    http::HttpClient(QString::fromStdString(video_info.thumbnail))
        .download([=, this](const auto& content) {
        QBuffer buffer;
        buffer.setData(content);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "JPG");
        const auto image = reader.read();
        if (image.isNull()) {
            return;
        }
        emit fetchThumbnailCompleted(id, QPixmap::fromImage(image));
            });
}

YtMusicInterop* YtMusic::interop() {
	if (interop_ != nullptr) {
        return interop_.get();
	}
    interop_ = MakeAlign<YtMusicInterop>();
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
        album["duration"].cast<std::string>(),
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

    return {
        playlist["id"].cast<std::string>(),
        playlist["privacy"].cast<std::string>(),
        playlist["title"].cast<std::string>(),
        extract_py_list<meta::Thumbnail>(playlist["thumbnails"]),
        extract_meta_artist(playlist["author"]),
        optional_key<std::string>(playlist, "year"),
        playlist["duration"].cast<std::string>(),
        playlist["trackCount"].cast<int>(),
        extract_py_list<playlist::Track>(playlist["tracks"]),
    };
}

std::vector<artist::Artist::Album> YtMusicInterop::getArtistAlbums(const std::string& channel_id, const std::string& params) const {
    const auto py_albums = impl_->get_ytmusic().attr("get_artist_albums")(channel_id, params);
    std::vector<artist::Artist::Album> albums;

    std::ranges::transform(py_albums, std::back_inserter(albums), [](py::handle album) {
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

video_info::VideoInfo YtMusicInterop::extractInfo(const std::string& video_id) const {
    const auto info = impl_->get_ytdl().attr("extract_info")(video_id, "download"_a = py::bool_(false));
    return {
        info["id"].cast<std::string>(),
        info["title"].cast<std::string>(),
        info.contains("artist") ? info["artist"].cast<std::string>() : "",
        info.contains("channel") ? info["channel"].cast<std::string>() : "",
        extract_py_list<video_info::Format>(info["formats"]),
        info["thumbnail"].cast<std::string>()
    };
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

    return {
        lyrics["source"].cast<std::optional<std::string>>(),
        lyrics["lyrics"].cast<std::string>()
    };
}

std::vector<search::SearchResultItem> YtMusicInterop::search(
    const std::string& query,
    const std::optional<std::string>& filter,
    const std::optional<std::string>& scope,
    const int limit,
    const bool ignore_spelling) const {
    std::vector<search::SearchResultItem> output;

    try {
        const auto results = impl_->get_ytmusic().attr("search")
            (
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
    }
    catch (const std::exception& e) {
        XAMP_LOG_D(impl_->logger, "{}", e.what());
    }
    
    return output;
}