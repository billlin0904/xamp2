#include <widget/extractfileworker.h>

#include <QDirIterator>
#include <QElapsedTimer>

#include <base/fastmutex.h>
#include <base/google_siphash.h>
#include <base/scopeguard.h>
#include <base/blake3hash.h>

#include <metadata/taglibmetareader.h>

#include <widget/database.h>
#include <widget/albumview.h>
#include <widget/databasefacade.h>

XAMP_DECLARE_LOG_NAME(ExtractFileWorker);

static size_t GetFileCount(const QString& dir, const QStringList& file_name_filters) {
    if (QFileInfo(dir).isFile()) {
        return 1;
    }

    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    size_t file_count = 0;
    while (itr.hasNext()) {
        if (QFileInfo(itr.next()).isFile()) {
            ++file_count;
        }
    }
    return file_count;
}

static Vector<QString> GetPathSortByFileCount(
    const Vector<QString>& paths,
    const QStringList& file_name_filters,
    std::function<void(size_t)>&& action) {
    FastMutex mutex;

    struct PathInfo {
        QString path;
        size_t file_count;
        size_t depth;
    };

    std::atomic<size_t> total_file_count = 0;
    Vector<PathInfo> path_infos;
    path_infos.reserve(paths.size());

    // Calculate file counts and depths in parallel
    Executor::ParallelFor(GetScanPathThreadPool(), paths, [&](const auto& path) {
        size_t file_count = GetFileCount(path, file_name_filters);
        {
	        size_t depth = path.count('/');
	        std::lock_guard<FastMutex> guard{ mutex };
            path_infos.push_back({ path, file_count, depth });
        }
        total_file_count += file_count;
        action(total_file_count);
        });

    // Sort path_infos based on file count and depth
    std::ranges::sort(path_infos, [](const auto& p1, const auto& p2) {
        if (p1.file_count != p2.file_count) {
            return p1.file_count < p2.file_count;
        }
        return p1.depth < p2.depth;
        });

    // Extract sorted paths
    Vector<QString> sorted_paths;
    sorted_paths.reserve(path_infos.size());
    for (const auto& path_info : path_infos) {
        sorted_paths.push_back(path_info.path);
    }

    return sorted_paths;
}

ExtractFileWorker::ExtractFileWorker() {
    logger_ = LoggerManager::GetInstance().GetLogger(kExtractFileWorkerLoggerName);
    GetScanPathThreadPool();
}

void ExtractFileWorker::ScanPathFiles(const PooledDatabasePtr& database_pool,
                                      HashMap<std::wstring, Vector<TrackInfo>>& album_groups,
                                      const QStringList& file_name_filters,
                                      const QString& dir,
                                      int32_t playlist_id) {
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    FloatMap<std::wstring, Vector<Path>> directory_files;
    Blake3Hash hasher;

    while (itr.hasNext()) {
        auto next_path = ToNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        auto directory = QFileInfo(next_path).dir().path().toStdWString();
        if (!directory_files.contains(directory)) {
            directory_files[directory].reserve(kReserveSize);
        }
        directory_files[directory].emplace_back(path);       
        hasher.Update(path);
    }

    if (directory_files.empty()) {
        if (!QFileInfo(dir).isFile()) {
            XAMP_LOG_DEBUG("Not found file: {}", String::ToString(dir.toStdWString()));
            return;
        }
        const auto next_path = ToNativeSeparators(dir);
        const auto path = next_path.toStdWString();
        const auto directory = QFileInfo(dir).dir().path().toStdWString();
        directory_files[directory].emplace_back(path);
        hasher.Update(path);
    }

    const auto path_hash = hasher.Get64bitHash();

    const auto db_hash = database_pool->Acquire()->GetParentPathHash(dir);
    if (db_hash == path_hash) {
        XAMP_LOG_DEBUG("Cache hit hash:{} path: {}", db_hash, String::ToString(dir.toStdWString()));
        emit FromDatabase(playlist_id, database_pool->Acquire()->GetPlayListEntityFromPathHash(db_hash));
        return;
    }

    if (is_stop_) {
        return;
    }

    Vector<Pair<std::wstring, Vector<Path>>> temp(directory_files.begin(), directory_files.end());

    std::ranges::sort(temp, [&](const auto& pair1, const auto& pair2) {
        return pair1.second.size() < pair2.second.size();
        });

    FastMutex mutex;

    for (const auto& pair : directory_files) {
        const auto& directory = pair.first;
        const auto& file_paths = pair.second;
        Executor::ParallelFor(GetScanPathThreadPool(), file_paths, [&](const auto& path) {
            if (is_stop_) {
                return;
            }
            try {
	            TaglibMetadataReader reader;
	            auto track_info = reader.Extract(path);
                track_info.parent_path_hash = path_hash;
                std::lock_guard<FastMutex> guard{ mutex };
                if (!album_groups.contains(track_info.album)) {
                    album_groups[track_info.album].reserve(kReserveSize);
                }
                album_groups[track_info.album].push_back(std::move(track_info));
            }
            catch (...) {
            }
            });
        ReadFilePath(StringFormat("Extract file {}", String::ToString(directory)));
    }
}

void ExtractFileWorker::ReadTrackInfo(QString const& file_path,
    int32_t playlist_id,
    bool is_podcast_mode) {
    Stopwatch sw;
    is_stop_ = false;
    constexpr QFlags<QDir::Filter> filter = QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs;
    QDirIterator itr(file_path, GetFileNameFilter(), filter);
    Blake3Hash hasher;

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

    auto database_pool = GetPooledDatabase();
    const auto path_hash = hasher.Get64bitHash();

    try {
        const auto db_hash = database_pool->Acquire()->GetParentPathHash(ToNativeSeparators(file_path));
        if (db_hash == path_hash) {
            XAMP_LOG_D(logger_, "Cache hit hash:{} path: {}", db_hash, String::ToString(file_path.toStdWString()));
            emit FromDatabase(playlist_id, database_pool->Acquire()->GetPlayListEntityFromPathHash(db_hash));
            return;
        }
    }
    catch (Exception const& e) {
        XAMP_LOG_E(logger_, "Failed to get parent path. {}", e.GetErrorMessage());
        return;
    }

    std::atomic<size_t> completed_work(0);

    emit ReadFileStart();

    auto file_count_paths = GetPathSortByFileCount(paths, GetFileNameFilter(), [this](auto total_file_count) {
        emit FoundFileCount(total_file_count);
        });

    const auto total_work = file_count_paths.size();

    FastMutex mutex;
    QElapsedTimer timer;
    timer.start();

    Executor::ParallelFor(GetBackgroundThreadPool(), file_count_paths, [&](const auto& path) {
        if (is_stop_) {
            return;
        }

        HashMap<std::wstring, Vector<TrackInfo>> album_groups;

        XAMP_ON_SCOPE_EXIT(
            const auto value = completed_work.load();
            emit ReadFileProgress((value * 100) / total_work);
            ++completed_work;
            
            const auto remaining_work = total_work - completed_work;

            qint64 elapsed_time = 0;
			{
                std::lock_guard<FastMutex> guard{ mutex };
                elapsed_time = timer.elapsed();
			}
            const auto remaining_time = (elapsed_time * remaining_work) / completed_work;
            emit CalculateEta(remaining_time);
        );

        try {
            ScanPathFiles(database_pool, album_groups, GetFileNameFilter(), path, playlist_id);
        }
        catch (Exception const& e) {
            XAMP_LOG_D(logger_, "Failed to scan path files! ", e.GetErrorMessage());
            return;
        }

        std::for_each(album_groups.begin(), album_groups.end(), [&](auto& album_tracks) {
            if (is_stop_) {
                return;
            }
            std::sort(album_tracks.second.begin(), album_tracks.second.end(), [](const auto& first, const auto& last) {
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
}

void ExtractFileWorker::OnCancelRequested() {
    is_stop_ = true;
}
