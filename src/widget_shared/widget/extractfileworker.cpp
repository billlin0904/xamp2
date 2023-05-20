#include <widget/extractfileworker.h>

#include <QSharedPointer>
#include <QDirIterator>

#include <base/fastmutex.h>
#include <base/google_siphash.h>
#include <base/scopeguard.h>

#include <widget/database.h>
#include <widget/albumview.h>
#include <widget/databasefacade.h>

using DirPathHash = GoogleSipHash<>;

XAMP_DECLARE_LOG_NAME(ExtractFileWorker);

static constexpr auto MakFilePathHash() noexcept -> DirPathHash {
    constexpr uint64_t kDirHashKey1 = 0x7720796f726c694bUL;
    constexpr uint64_t kDirHashKey2 = 0x2165726568207361UL;

    return DirPathHash(kDirHashKey1, kDirHashKey2);
}

static size_t GetFileCount(const QString& dir, const QStringList& file_name_filters) {
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    size_t file_count = 0;
    while (itr.hasNext()) {
        if (QFileInfo(itr.next()).isFile()) {
            ++file_count;
        }
    }
    return file_count;
}

static Vector<QString> GetPathSortByFileCount(const Vector<QString>& paths,
    const QStringList& file_name_filters,
    std::function<void(size_t)>&& action) {
    FastMutex mutex;
    OrderedMap<size_t, Vector<QString>> file_count_map;
    OrderedMap<QString, size_t> path2file_count_map;
    size_t total_file_count = 0;

    Executor::ParallelFor(GetScanPathThreadPool(), paths, [&](const auto& path) {
        std::lock_guard<FastMutex> guard{ mutex };
        auto file_count = GetFileCount(path, file_name_filters);
        file_count_map[file_count].push_back(path);
        path2file_count_map[path] = file_count;
        total_file_count += file_count;
        action(total_file_count);
        });

    auto comparePaths = [&](const QString& path1, const QString& path2) {
        const auto file_count1 = path2file_count_map[path1];
        const auto file_count2 = path2file_count_map[path2];
        const auto depth1 = path1.count('/');
        const auto depth2 = path2.count('/');
        if (file_count1 != file_count2) {
            return file_count1 < file_count2;
        }
        else {
            return depth1 < depth2;
        }
    };

    Vector<QString> file_count_paths;
    file_count_paths.reserve(file_count_map.size());
    for (auto& pair : file_count_map) {
        auto& paths = pair.second;
        std::sort(paths.begin(), paths.end(), comparePaths);
        file_count_paths.insert(file_count_paths.end(), paths.begin(), paths.end());
    }

    return file_count_paths;
}

ExtractFileWorker::ExtractFileWorker() {
    logger_ = LoggerManager::GetInstance().GetLogger(kExtractFileWorkerLoggerName);
}

void ExtractFileWorker::ScanPathFiles(HashMap<std::wstring, ForwardList<TrackInfo>>& album_groups,
    const QStringList& file_name_filters,
    const QString& dir,
    int32_t playlist_id,
    bool is_podcast_mode) {
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    FloatMap<std::wstring, Vector<Path>> directory_files;
    auto hasher = MakFilePathHash();

    while (itr.hasNext()) {
        auto next_path = ToNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        auto directory = QFileInfo(next_path).dir().path().toStdWString();
        directory_files[directory].push_back(path);
        hasher.Update(path);
    }

    if (directory_files.empty()) {
        if (!QFileInfo(dir).isFile()) {
            XAMP_LOG_DEBUG("Not found file: {}", String::ToString(dir.toStdWString()));
            return;
        }
        auto next_path = ToNativeSeparators(dir);
        const auto path = next_path.toStdWString();
        auto directory = QFileInfo(dir).dir().path().toStdWString();
        directory_files[directory].push_back(path);
        hasher.Update(path);
    }

    const auto path_hash = hasher.GetHash();

    const auto db_hash = qDatabase.GetParentPathHash(dir);
    if (db_hash == path_hash) {
        XAMP_LOG_DEBUG("Cache hit hash:{} path: {}", db_hash, String::ToString(dir.toStdWString()));
        emit FromDatabase(playlist_id, qDatabase.GetPlayListEntityFromPathHash(db_hash));
        return;
    }

    if (is_stop_) {
        return;
    }

    Vector<Pair<std::wstring, Vector<Path>>> temp(directory_files.begin(), directory_files.end());

    std::sort(temp.begin(), temp.end(), [&](const auto& pair1, const auto& pair2) {
        return pair1.second.size() < pair2.second.size();
        });

    FastMutex mutex;

    for (const auto& pair : directory_files) {
        const auto& directory = pair.first;
        const auto& file_paths = pair.second;
        Stopwatch sw;
        Executor::ParallelFor(GetScanPathThreadPool(), file_paths, [&](const auto& path) {
            if (is_stop_) {
                return;
            }
            auto reader = MakeMetadataReader();
            try {
                auto track_info = reader->Extract(path);
                track_info.parent_path_hash = path_hash;
                std::lock_guard<FastMutex> guard{ mutex };
                album_groups[track_info.album].emplace_front(std::move(track_info));
            }
            catch (...) {
            }
            //XAMP_LOG_DEBUG("Extract file path : {}", String::ToString(path.wstring()));
            });
        //XAMP_LOG_DEBUG("Extract file {} count:{} ({} secs)", String::ToString(directory), file_paths.size(), sw.ElapsedSeconds());
    }
}

void ExtractFileWorker::ReadTrackInfo(QString const& file_path,
    int32_t playlist_id,
    bool is_podcast_mode) {
    Stopwatch sw;
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

    XAMP_ON_SCOPE_EXIT(
        XAMP_LOG_D(logger_, "Finish to read track info.");
    );

    const auto path_hash = hasher.GetHash();

    try {
        const auto db_hash = qDatabase.GetParentPathHash(ToNativeSeparators(file_path));
        if (db_hash == path_hash) {
            XAMP_LOG_D(logger_, "Cache hit hash:{} path: {}", db_hash, String::ToString(file_path.toStdWString()));
            emit FromDatabase(playlist_id, qDatabase.GetPlayListEntityFromPathHash(db_hash));
            return;
        }
    }
    catch (Exception const& e) {
        XAMP_LOG_E(logger_, "Failed to get parent path. {}", e.GetErrorMessage());
        return;
    }

    const auto file_name_filters = GetFileNameFilter();
    std::atomic<size_t> progress(0);

    emit ReadFileStart();

    auto file_count_paths = GetPathSortByFileCount(paths, file_name_filters, [this](auto total_file_count) {
        emit FoundFileCount(total_file_count);
        });

    Executor::ParallelFor(GetBackgroundThreadPool(), file_count_paths, [&](const auto& path) {
        if (is_stop_) {
            return;
        }

        HashMap<std::wstring, ForwardList<TrackInfo>> album_groups;

        XAMP_ON_SCOPE_EXIT(
            auto value = progress.load();
            emit ReadFileProgress((value * 100) / file_count_paths.size());
            ++progress;
        );

        try {
            ScanPathFiles(album_groups, file_name_filters, path, playlist_id, is_podcast_mode);
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
            emit InsertDatabase(album_tracks.second, playlist_id, is_podcast_mode);
            });
        });

    emit ReadCompleted();
    XAMP_LOG_DEBUG("Extract file ({} secs)", sw.ElapsedSeconds());
}

void ExtractFileWorker::OnExtractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode) {
    ReadTrackInfo(file_path, playlist_id, false);
    OnLoadAlbumCoverCache();
}

void ExtractFileWorker::OnLoadAlbumCoverCache() {
    QList<QString> cover_ids;
    cover_ids.reserve(LazyLoadingModel::kMaxBatchSize);

    qDatabase.ForEachAlbumCover([&cover_ids](const auto& cover_id) {
        cover_ids.push_back(cover_id);
        }, LazyLoadingModel::kMaxBatchSize);

    try {
        Executor::ParallelFor(GetBackgroundThreadPool(), cover_ids, [](const auto& cover_id) {
            qPixmapCache.GetCover(AlbumViewStyledDelegate::kAlbumCacheTag, cover_id);
            });
    }
    catch (const std::exception& e) {
        XAMP_LOG_ERROR("OnLoadAlbumCoverCache: {}", e.what());
    }
}