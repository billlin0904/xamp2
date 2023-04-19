#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>

#include <QDirIterator>
#include <qguiapplication.h>
#include <QtConcurrent/qtconcurrentrun.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/google_siphash.h>
#include <base/scopeguard.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>

#include <stream/avfilestream.h>

#include <thememanager.h>

#include <widget/widget_shared.h>
#include <widget/http.h>
#include <widget/xmessagebox.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/imagecache.h>
#include <widget/databasefacade.h>

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

#define IGNORE_ANY_EXCEPTION(expr) \
    do {\
		try {\
			expr;\
		}\
		catch (...) {}\
    } while (false)

CoverArtReader::CoverArtReader()
    : cover_reader_(MakeMetadataReader()) {
}

QPixmap CoverArtReader::GetEmbeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QPixmap CoverArtReader::GetEmbeddedCover(const TrackInfo& track_info) const {
    return GetEmbeddedCover(track_info.file_path);
}

DatabaseFacade::DatabaseFacade(QObject* parent)
    : QObject(parent) {
    logger_ = LoggerManager::GetInstance().GetLogger(kDatabaseFacadeLoggerName);
    QObject::connect(&timer_, &QTimer::timeout, &event_loop_, &QEventLoop::quit);
    timer_.setInterval(100);
    timer_.start();
}

using DirPathHash = GoogleSipHash<>;

static auto MakFilePathHash() noexcept -> DirPathHash {
    static constexpr uint64_t kDirHashKey1 = 0x7720796f726c694bUL;
    static constexpr uint64_t kDirHashKey2 = 0x2165726568207361UL;

    return DirPathHash(kDirHashKey1, kDirHashKey2);
}

void DatabaseFacade::ScanPathFiles(const QStringList& file_name_filters,
    const QString& dir,
    int32_t playlist_id,
    bool is_podcast_mode) {
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

    auto hasher = MakFilePathHash();
    ForwardList<Path> paths;

    while (itr.hasNext()) {
        auto next_path = ToNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        paths.push_front(path);
        hasher.Update(path);
    }

    if (paths.empty()) {
        if (!QFileInfo(dir).isFile()) {
            XAMP_LOG_DEBUG("Not found file: {}", String::ToString(dir.toStdWString()));
            return;
        }
        const auto path = dir.toStdWString();
        paths.push_front(path);
        hasher.Update(path);
    }

    const auto path_hash = hasher.GetHash();

    const auto db_hash = qDatabase.GetParentPathHash(dir);
    if (db_hash == path_hash) {
        XAMP_LOG_DEBUG("Cache hit hash:{} path: {}", db_hash, String::ToString(dir.toStdWString()));
        emit FromDatabase(qDatabase.GetPlayListEntityFromPathHash(db_hash));
        return;
    }

    auto reader = MakeMetadataReader();

    HashMap<std::wstring, ForwardList<TrackInfo>> album_groups;
    std::for_each(paths.begin(), paths.end(), [&](auto& path) {
        if (is_stop_) {
            return;
        }

        Stopwatch sw;
        auto track_info = reader->Extract(path);
        constexpr auto kTagLibInvalidBitRate = 1;
        if (track_info.bit_rate == kTagLibInvalidBitRate || track_info.duration == 0.0) {
            try {
                AvFileStream stream;
                stream.OpenFile(path);
                track_info.duration = stream.GetDuration();
                track_info.bit_rate = stream.GetBitRate();
            }
            catch (...) {
            }
        }
        XAMP_LOG_DEBUG("Extract file {} secs", sw.ElapsedSeconds());
        album_groups[track_info.album].push_front(std::move(track_info));
        });

    int32_t total_tracks = 0;
    std::for_each(album_groups.begin(), album_groups.end(), [&](auto& tracks) {
        if (is_stop_) {
            return;
        }

        std::for_each(tracks.second.begin(), tracks.second.end(), [&](auto& track) {
            track.parent_path_hash = path_hash;
            });

        tracks.second.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        total_tracks += std::distance(tracks.second.begin(), tracks.second.end());
        });

    std::for_each(album_groups.begin(), album_groups.end(), [&](auto& album_tracks) {
        if (is_stop_) {
            return;
        }
        InsertTrackInfo(album_tracks.second, playlist_id, is_podcast_mode);
        });

    emit ReadCompleted(album_groups.size(), total_tracks);
}

void DatabaseFacade::ReadTrackInfo(QString const& file_path,
    int32_t playlist_id,
    bool is_podcast_mode) {

    constexpr QFlags<QDir::Filter> filter = QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs;
    QDirIterator itr(file_path, GetFileNameFilter(), filter);

    auto hasher = MakFilePathHash();

    Vector<QString> paths;
    paths.reserve(1024);

    while (itr.hasNext()) {
        if (is_stop_) {
            return;
        }
        auto path = ToNativeSeparators(itr.next());
        hasher.Update(path.toStdWString());
        paths.push_back(path);
    }

    if (paths.empty()) {
        paths.push_back(file_path);
    }

    const int path_size = std::distance(paths.begin(), paths.end());
    emit ReadFileStart(path_size);

    XAMP_ON_SCOPE_EXIT(
        emit ReadFileEnd();
		XAMP_LOG_D(logger_, "Finish to read track info.");
    );

    const auto path_hash = hasher.GetHash();

    try {
        const auto db_hash = qDatabase.GetParentPathHash(ToNativeSeparators(file_path));
        if (db_hash == path_hash) {
            XAMP_LOG_D(logger_, "Cache hit hash:{} path: {}", db_hash, String::ToString(file_path.toStdWString()));
            emit FromDatabase(qDatabase.GetPlayListEntityFromPathHash(db_hash));
            return;
        }
    }
    catch (Exception const& e) {
        XAMP_LOG_E(logger_, "Failed to get parent path. {}", e.GetErrorMessage());
        return;
    }

    const auto file_name_filters = GetFileNameFilter();
    int progress(0);

    for (const auto path : paths) {
        if (is_stop_) {
            return;
        }

        try {
            ScanPathFiles(file_name_filters, path, playlist_id, is_podcast_mode);
        }
        catch (Exception const& e) {
            XAMP_LOG_D(logger_, "Failed to scan path files! ", e.GetErrorMessage());
        }

        emit ReadFileProgress(progress);
        event_loop_.exec();
        ++progress;
    }
}

void DatabaseFacade::FindAlbumCover(int32_t album_id,
    const QString& album, 
    const std::wstring& file_path,
    const CoverArtReader& reader) {
	const auto cover_id = qDatabase.GetAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

    std::wstring find_file_path;
	const auto first_file_path = qDatabase.GetAlbumFirstMusicFilePath(album_id);
    if (!first_file_path) {
        find_file_path = file_path;
    } else {
        find_file_path = (*first_file_path).toStdWString(); 
    }

	auto cover = reader.GetEmbeddedCover(find_file_path);
    if (!cover.isNull()) {
        qDatabase.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
        return;
    }

	cover = ImageCache::ScanImageFromDir(QString::fromStdWString(file_path));
    if (!cover.isNull()) {
        qDatabase.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
    }
}

void DatabaseFacade::AddTrackInfo(const ForwardList<TrackInfo>& result,
    int32_t playlist_id,
    bool is_podcast) {
    const Stopwatch watch;
	const CoverArtReader reader;
	const auto total_tracks = std::distance(result.begin(), result.end());
    auto num_track = 0;
    HashMap<QString, int32_t> artist_id_cache;
    HashMap<QString, int32_t> album_id_cache;
	for (const auto& track_info : result) {
        auto file_path = QString::fromStdWString(track_info.file_path);
        auto album = QString::fromStdWString(track_info.album);
        auto artist = QString::fromStdWString(track_info.artist);
		auto disc_id = QString::fromStdString(track_info.disc_id);

        QPixmap cover;
		if (album.isEmpty()) {
			album = tr("Unknown album");
			// todo: 如果有內建圖片就把當作一張專輯.
			cover = reader.GetEmbeddedCover(track_info);
			if (!cover.isNull()) {
				album = QString::fromStdWString(track_info.file_name_no_ext);
			}
		}

		if (artist.isEmpty() || is_podcast) {
			artist = tr("Unknown artist");
		}
        
        const auto music_id = qDatabase.AddOrUpdateMusic(track_info);      
        int32_t artist_id = 0;
        if (!artist_id_cache.contains(artist)) {
            artist_id = qDatabase.AddOrUpdateArtist(artist);
            artist_id_cache[artist] = artist_id;
        } else {
            artist_id = artist_id_cache[artist];
        }

        XAMP_EXPECTS(artist_id != 0);

        int32_t album_id = 0;
        if (!album_id_cache.contains(album + artist)) {
            album_id = qDatabase.AddOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                is_podcast,
                disc_id);
            album_id_cache[album + artist] = album_id;
        } else {
            album_id = album_id_cache[album + artist];
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != -1) {
			qDatabase.AddMusicToPlaylist(music_id, playlist_id, album_id);
		}

        IGNORE_ANY_EXCEPTION(qDatabase.AddOrUpdateAlbumMusic(album_id, artist_id, music_id));
        IGNORE_ANY_EXCEPTION(qDatabase.AddOrUpdateAlbumArtist(album_id, artist_id));

        if (!cover.isNull()) {
            qDatabase.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
        } else {
            FindAlbumCover(album_id, album, track_info.file_path, reader);
        }
                        
        emit ReadCurrentFilePath(file_path, total_tracks, num_track++);
        if (!is_podcast) {
            event_loop_.exec();
        }        
	}
    XAMP_LOG_DEBUG("Thread:{} Add TrackInfo success! {:.2f} sesc",
        QThread::currentThreadId(), watch.ElapsedSeconds());
}

void DatabaseFacade::InsertTrackInfo(const ForwardList<TrackInfo>& result, 
    int32_t playlist_id, 
    bool is_podcast_mode) {
    try {
        if (!qDatabase.transaction()) {
            XAMP_LOG_DEBUG("Failed to begin transaction!");
        }
        AddTrackInfo(result, playlist_id, is_podcast_mode);       
        if (!qDatabase.commit()) {
            XAMP_LOG_DEBUG("Failed to commit!");
        }
        return;
    }
    catch (Exception const& e) {
        XAMP_LOG_DEBUG("Failed to add track info({})!", e.GetErrorMessage());
    }
    catch (std::exception const& e) {
        XAMP_LOG_DEBUG("Failed to add track info({})!", e.what());
    }
    catch (...) {
        XAMP_LOG_DEBUG("Failed to add track info!");
    }
    qDatabase.rollback();
}

TrackInfo GetTrackInfo(QString const& file_path) {
    const Path path(file_path.toStdWString());
    const auto reader = MakeMetadataReader();
    return reader->Extract(path);
}

QString GetFileDialogFileExtensions() {
    QString exts(qTEXT("("));
    for (const auto& file_ext : GetSupportFileExtensions()) {
        exts += qTEXT("*") + QString::fromStdString(file_ext);
        exts += qTEXT(" ");
    }
    exts += qTEXT(")");
    return exts;
}

QStringList GetFileNameFilter() {
    QStringList name_filter;
    for (auto& file_ext : GetSupportFileExtensions()) {
        name_filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
    }
    return name_filter;
}
