#include <widget/databasefacade.h>
#include <widget/backgroundworker.h>

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
#include <base/fastmutex.h>

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

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

#define IGNORE_ANY_EXCEPTION(expr) \
    do {\
		try {\
			expr;\
		}\
		catch (...) {}\
    } while (false)

using DirPathHash = GoogleSipHash<>;

static constexpr auto MakFilePathHash() noexcept -> DirPathHash {
    constexpr uint64_t kDirHashKey1 = 0x7720796f726c694bUL;
    constexpr uint64_t kDirHashKey2 = 0x2165726568207361UL;

    return DirPathHash(kDirHashKey1, kDirHashKey2);
}

static QSet<QString> GetAlbumCategories(const QString& album) {
    QRegularExpression regex(
        R"((final fantasy \b|piano collections|vocal collection|soundtrack|best|complete|collection)(?:(?: \[.*\])|(?: - .*))?)",
        QRegularExpression::CaseInsensitiveOption);

    QSet<QString> categories;
    auto it = regex.globalMatch(album);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString category = match.captured(1).toLower().trimmed();
        categories.insert(category);
    }
    return categories;
}

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
}

QStringList DatabaseFacade::NormalizeGenre(const QString& genre) {
    static const QString kJop = "jpop";
    QStringList normalizedTags;

    if (genre.isEmpty()) return normalizedTags;

    if (genre.length() == 1 && genre[0] == ' ') return normalizedTags;

    auto tags = genre.split(QRegExp("\\s*,\\s*"), Qt::SkipEmptyParts);

    for (auto tag : tags) {
        tag = tag.trimmed();
        if (tag.contains(QRegExp("[/|()&]"))) {
            QStringList subTags = tag.split(QRegExp("[/|()&]"), Qt::SkipEmptyParts);
            for (auto s : subTags) {         
                s = s.trimmed().toLower();
                if (s.length() == 1) {
                    continue;
                }                
                if (s == kJop) {
                    s = "j-pop";
                }
                normalizedTags.append(s);
            }            
        }
        else {
            auto s = tag.trimmed().toLower();
            if (s.length() == 1) {
                continue;
            }
            if (s == kJop) {
                s = "j-pop";
            }
            normalizedTags.append(s);
        }
    }

    return normalizedTags;
}

void DatabaseFacade::ScanPathFiles(BackgroundWorker* worker,
    HashMap<std::wstring, ForwardList<TrackInfo>>& album_groups,
    const QStringList& file_name_filters,
    const QString& dir,
    int32_t playlist_id,
    bool is_podcast_mode) {
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

    auto hasher = MakFilePathHash();

    Vector<Path> paths;
    paths.reserve(kReserveSize);

    while (itr.hasNext()) {
        auto next_path = ToNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        paths.push_back(path);
        hasher.Update(path);
    }

    if (paths.empty()) {
        if (!QFileInfo(dir).isFile()) {
            XAMP_LOG_DEBUG("Not found file: {}", String::ToString(dir.toStdWString()));
            return;
        }
        const auto path = dir.toStdWString();
        paths.push_back(path);
        hasher.Update(path);
    }

    const auto path_hash = hasher.GetHash();

    const auto db_hash = qDatabase.GetParentPathHash(dir);
    if (db_hash == path_hash) {
        XAMP_LOG_DEBUG("Cache hit hash:{} path: {}", db_hash, String::ToString(dir.toStdWString()));
        emit worker->FromDatabase(playlist_id, qDatabase.GetPlayListEntityFromPathHash(db_hash));
        return;
    }
   
   if (is_stop_) {
       return;
   }

   FastMutex mutex;

   XAMP_LOG_DEBUG("Extract file count: {}", paths.size());

   Executor::ParallelFor(GetScanPathThreadPool(), paths, [&](auto& path) {
       auto reader = MakeMetadataReader();
       try {
           std::lock_guard<FastMutex> lock{ mutex };           
           auto track_info = reader->Extract(path);           
           track_info.parent_path_hash = path_hash;
           album_groups[track_info.album].emplace_front(std::move(track_info));
       }
       catch (...) {
       }
       });
}

void DatabaseFacade::ReadTrackInfo(BackgroundWorker* worker,
    QString const& file_path,
    int32_t playlist_id,
    bool is_podcast_mode) {

    constexpr QFlags<QDir::Filter> filter = QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs;
    QDirIterator itr(file_path, GetFileNameFilter(), filter);

    auto hasher = MakFilePathHash();
    
    Vector<QString> paths;
    paths.reserve(kReserveSize);

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
    XAMP_ON_SCOPE_EXIT(
		XAMP_LOG_D(logger_, "Finish to read track info.");
    );

    const auto path_hash = hasher.GetHash();

    try {
        const auto db_hash = qDatabase.GetParentPathHash(ToNativeSeparators(file_path));
        if (db_hash == path_hash) {
            XAMP_LOG_D(logger_, "Cache hit hash:{} path: {}", db_hash, String::ToString(file_path.toStdWString()));
            emit worker->FromDatabase(playlist_id, qDatabase.GetPlayListEntityFromPathHash(db_hash));
            return;
        }
    }
    catch (Exception const& e) {
        XAMP_LOG_E(logger_, "Failed to get parent path. {}", e.GetErrorMessage());
        return;
    }

    const auto file_name_filters = GetFileNameFilter();
    std::atomic<size_t> progress(0);

    emit worker->ReadFileStart();

    Executor::ParallelFor(GetBackgroundThreadPool(), paths, [&](const auto& path) {
        if (is_stop_) {
			return;
		}

        XAMP_LOG_DEBUG("Process {}", path.toStdString());

		HashMap<std::wstring, ForwardList<TrackInfo>> album_groups;

        XAMP_ON_SCOPE_EXIT(
            auto value = progress.load();
            emit worker->ReadFileProgress((value * 100) / paths.size());
            ++progress;
            XAMP_LOG_DEBUG("Progress {}%", (value * 100) / paths.size());
        );

        try {
			ScanPathFiles(worker, album_groups, file_name_filters, path, playlist_id, is_podcast_mode);
		}
        catch (Exception const& e) {
			XAMP_LOG_D(logger_, "Failed to scan path files! ", e.GetErrorMessage());
            return;
		}

        std::for_each(album_groups.begin(), album_groups.end(), [&](auto& album_tracks) {
            if (is_stop_) {
                return;
            }
            album_tracks.second.sort([](const auto& first, const auto& last) {
                return first.track < last.track;
            });
            emit worker->InsertDatabase(album_tracks.second, playlist_id, is_podcast_mode);            
            });        
	});

    emit worker->ReadCompleted();
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

	cover = ImageCache::ScanCoverFromDir(QString::fromStdWString(file_path));
    if (!cover.isNull()) {
        qDatabase.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
    }
    else {
        qDatabase.SetAlbumCover(album_id, album, qPixmapCache.GetUnknownCoverId());
    }
}

void DatabaseFacade::AddTrackInfo(const ForwardList<TrackInfo>& result,
    int32_t playlist_id,
    bool is_podcast) {
    const Stopwatch watch;
	const CoverArtReader reader;	

    constexpr ConstLatin1String kHiRes("hires");    
    constexpr ConstLatin1String kDsdCategory("dsd");
    constexpr auto k24Bit96KhzBitRate = 4608;

    const std::wstring kDffExtension(L".dff");
    const std::wstring kDsfExtension(L".dsf");

    HashMap<QString, int32_t> artist_id_cache;
    HashMap<QString, int32_t> album_id_cache;
    
    auto album_year = result.front().year;
    auto album_genre = NormalizeGenre(QString::fromStdWString(result.front().genre)).join(",");
    
	for (const auto& track_info : result) {        
        auto file_path = QString::fromStdWString(track_info.file_path);
        auto album = QString::fromStdWString(track_info.album);
        auto artist = QString::fromStdWString(track_info.artist);
		auto disc_id = QString::fromStdString(track_info.disc_id);

        const auto is_file_path = IsFilePath(track_info.file_path);

        QPixmap cover;
		if (is_file_path && album.isEmpty()) {
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

        auto album_id = qDatabase.GetAlbumId(album);
        if (album_id == kInvalidDatabaseId) {
            album_id = qDatabase.AddOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                album_year,
                is_podcast,
                disc_id,
                album_genre);
            album_id_cache[album] = album_id;
            for (const auto &category : GetAlbumCategories(album)) {
                qDatabase.AddOrUpdateAlbumCategory(album_id, category);
            }
            
            if (track_info.file_ext == kDsfExtension || track_info.file_ext == kDffExtension) {
                qDatabase.AddOrUpdateAlbumCategory(album_id, kDsdCategory);
            }
            else if (track_info.bit_rate >= k24Bit96KhzBitRate) {
				qDatabase.AddOrUpdateAlbumCategory(album_id, kHiRes);
			}            
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != -1) {
			qDatabase.AddMusicToPlaylist(music_id, playlist_id, album_id);
		}

        IGNORE_ANY_EXCEPTION(qDatabase.AddOrUpdateAlbumMusic(album_id, artist_id, music_id));
        IGNORE_ANY_EXCEPTION(qDatabase.AddOrUpdateAlbumArtist(album_id, artist_id));

        if (!is_file_path) {
            continue;
        }

        if (!cover.isNull()) {
            qDatabase.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
        } else {
            FindAlbumCover(album_id, album, track_info.file_path, reader);
        }
	}
}

void DatabaseFacade::InsertTrackInfo(const ForwardList<TrackInfo>& result, 
    int32_t playlist_id, 
    bool is_podcast_mode) {
    try {
        if (!qDatabase.transaction()) {
            XAMP_LOG_DEBUG("Failed to begin transaction!");
            return;
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
