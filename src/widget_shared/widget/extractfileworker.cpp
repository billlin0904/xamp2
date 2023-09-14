#include <widget/extractfileworker.h>

#include <QDirIterator>
#include <QElapsedTimer>

#include <base/fastmutex.h>
#include <base/scopeguard.h>

#include <metadata/taglibmetareader.h>

#include <widget/database.h>
#include <widget/albumview.h>
#include <widget/databasefacade.h>

XAMP_DECLARE_LOG_NAME(ExtractFileWorker);

namespace {
    size_t GetFileCount(const QString& dir, const QStringList& file_name_filters) {
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

    Vector<QString> GetPathSortByFileCount(
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
        for (const auto & path : paths) {
            path_infos.push_back({ path, 0, 0 });
        }

        Executor::ParallelFor(GetScanPathThreadPool(), path_infos, [&](auto& path_info) {
            path_info.file_count = GetFileCount(path_info.path, file_name_filters);
            path_info.depth = path_info.path.count('/');
            total_file_count += path_info.file_count;
            action(total_file_count);
            });

        Vector<QString> sorted_paths;

        if (total_file_count == 0) {
            return sorted_paths;
        }

        // Sort path_infos based on file count and depth
        std::ranges::sort(path_infos, [](const auto& p1, const auto& p2) {
            if (p1.file_count != p2.file_count) {
                return p1.file_count < p2.file_count;
            }
            return p1.depth < p2.depth;
            });

        // Extract sorted paths

        sorted_paths.reserve(path_infos.size());
        for (const auto& path_info : path_infos) {
            sorted_paths.push_back(path_info.path);
        }

        return sorted_paths;
    }
}

ExtractFileWorker::ExtractFileWorker() {
    logger_ = LoggerManager::GetInstance().GetLogger(kExtractFileWorkerLoggerName);
    GetScanPathThreadPool();
}

HashMap<std::wstring, Vector<TrackInfo>> ExtractFileWorker::ScanPathFiles(const PooledDatabasePtr& database_pool,
                                      const QStringList& file_name_filters,
                                      const QString& dir) {
    HashMap<std::wstring, Vector<TrackInfo>> album_groups;
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    FloatMap<std::wstring, Vector<Path>> directory_files;

    while (itr.hasNext()) {
        auto next_path = ToNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        auto directory = QFileInfo(next_path).dir().path().toStdWString();
        if (!directory_files.contains(directory)) {
            directory_files[directory].reserve(kReserveSize);
        }
        directory_files[directory].emplace_back(path);       
    }

    if (directory_files.empty()) {
        if (!QFileInfo(dir).isFile()) {
            XAMP_LOG_DEBUG("Not found file: {}", String::ToString(dir.toStdWString()));
            return album_groups;
        }
        const auto next_path = ToNativeSeparators(dir);
        const auto path = next_path.toStdWString();
        const auto directory = QFileInfo(dir).dir().path().toStdWString();
        directory_files[directory].emplace_back(path);
    }

    if (is_stop_) {
        return album_groups;
    }

    for (const auto& pair : directory_files) {
        const auto& directory = pair.first;

        for (const auto & path : pair.second) {
            if (is_stop_) {
                return album_groups;
            }
            try {
                TaglibMetadataReader reader;
                auto track_info = reader.Extract(path);
                if (track_info.album) {
                    if (!album_groups.contains(track_info.album.value())) {
                        album_groups[track_info.album.value()].reserve(kReserveSize);
                    }
                    album_groups[track_info.album.value()].push_back(std::move(track_info));
                }
            }
            catch (...) { }
        }        
        emit ReadFilePath(StringFormat("Extract directory {}", String::ToString(directory)));
    }
    return album_groups;
}

void ExtractFileWorker::ReadTrackInfo(QString const& file_path,
    int32_t playlist_id,
    bool is_podcast_mode) {
    Stopwatch sw;
    is_stop_ = false;
    constexpr QFlags<QDir::Filter> filter = QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs;
    QDirIterator itr(file_path, GetFileNameFilter(), filter);

    Vector<QString> paths;
    paths.reserve(kReserveSize);

    while (itr.hasNext()) {
        if (is_stop_) {
            return;
        }
        auto path = ToNativeSeparators(itr.next());
        paths.push_back(path);
    }

    if (paths.empty()) {
        paths.push_back(file_path);
    }

    auto database_pool = GetPooledDatabase();

    emit ReadFileStart();

    XAMP_ON_SCOPE_EXIT(
        emit ReadCompleted();
		XAMP_LOG_D(logger_, "Finish to read track info. ({} secs)", sw.ElapsedSeconds());
    );

    std::atomic<size_t> completed_work(0);
   
    auto file_count_paths = GetPathSortByFileCount(paths, GetFileNameFilter(), [this](auto total_file_count) {
        emit FoundFileCount(total_file_count);
    });

    const auto total_work = file_count_paths.size();
    if (total_work == 0) {
        XAMP_LOG_DEBUG("Not found file: {}", String::ToString(file_path.toStdWString()));
        return;
    }

    FastMutex mutex;
    QElapsedTimer timer;
    timer.start();

    Executor::ParallelFor(GetBackgroundThreadPool(), file_count_paths, [&](const auto& path) {
        if (is_stop_) {
            return;
        }

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
            auto album_groups = ScanPathFiles(database_pool, GetFileNameFilter(), path);
            std::for_each(album_groups.begin(), album_groups.end(), [&](auto& album_tracks) {
                if (is_stop_) {
                    return;
                }
                std::sort(album_tracks.second.begin(), album_tracks.second.end(), [](const auto& first, const auto& last) {
                    return first.track < last.track;
                    });
                emit InsertDatabase(album_tracks.second, playlist_id, is_podcast_mode);
                });
        }
        catch (Exception const& e) {
            XAMP_LOG_D(logger_, "Failed to scan path files! ", e.GetErrorMessage());
        }
        });
}

void ExtractFileWorker::OnExtractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode) {
    ReadTrackInfo(file_path, playlist_id, false);
}

void ExtractFileWorker::OnCancelRequested() {
    is_stop_ = true;
}
