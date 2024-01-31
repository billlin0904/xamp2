#include <widget/worker/filesystemworker.h>

#include <QDirIterator>

#include <base/scopeguard.h>

#include <metadata/taglibmetareader.h>

#include <widget/albumview.h>
#include <widget/util/ui_utilts.h>

XAMP_DECLARE_LOG_NAME(FileSystemWorker);

namespace {
    struct PathInfo {
        size_t file_count;
        size_t depth;
        QString path;
    };

    std::pair<size_t, Vector<PathInfo>> getPathSortByFileCount(
        const Vector<QString>& paths,
        const QStringList& file_name_filters,
        std::function<void(size_t)>&& action) {
        std::atomic<size_t> total_file_count = 0;
        Vector<PathInfo> path_infos;
        path_infos.reserve(paths.size());

        // Calculate file counts and depths in parallel
        for (const auto & path : paths) {
            path_infos.push_back({ 0, 0, path });
        }

        Executor::ParallelFor(GetBackgroundThreadPool(), path_infos, [&](auto& path_info) {
            path_info.file_count = getFileCount(path_info.path, file_name_filters);
            path_info.depth = path_info.path.count('/');
            total_file_count += path_info.file_count;
            action(total_file_count);
            });

        std::erase_if(path_infos,
                      [](const auto& info) { return info.file_count == 0; });

        if (total_file_count == 0) {
            return std::make_pair(total_file_count.load(), path_infos);
        }

        // Sort path_infos based on file count and depth
        std::ranges::sort(path_infos, [](const auto& p1, const auto& p2) {
            if (p1.file_count != p2.file_count) {
                return p1.file_count < p2.file_count;
            }
            return p1.depth < p2.depth;
            });
        return std::make_pair(total_file_count.load(), path_infos);
    }
}

FileSystemWorker::FileSystemWorker()
	: watcher_(this) {
    logger_ = LoggerManager::GetInstance().GetLogger(kFileSystemWorkerLoggerName);
    GetBackgroundThreadPool();
    (void)QObject::connect(&watcher_,
        &FileSystemWatcher::directoryChanged,
        this,
        [this](const auto &dir) {
            onExtractFile(dir, -1);
        });
}

void FileSystemWorker::onSetWatchDirectory(const QString& dir) {
    watcher_.addPath(dir);
}

size_t FileSystemWorker::scanPathFiles(const QStringList& file_name_filters,
                                      int32_t playlist_id,
                                      const QString& dir) {
    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    FloatMap<std::wstring, Vector<Path>> directory_files;

    while (itr.hasNext()) {
        if (is_stop_) {
            return 0;
        }
        auto next_path = toNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        auto directory = QFileInfo(next_path).dir().path().toStdWString();
        if (!directory_files.contains(directory)) {
            directory_files[directory].reserve(kReserveFilePathSize);
        }
        directory_files[directory].emplace_back(path);       
    }

    if (directory_files.empty()) {
        if (!QFileInfo(dir).isFile()) {
            XAMP_LOG_DEBUG("Not found file: {}", String::ToString(dir.toStdWString()));
            return 0;
        }
        const auto next_path = toNativeSeparators(dir);
        const auto path = next_path.toStdWString();
        const auto directory = QFileInfo(dir).dir().path().toStdWString();
        directory_files[directory].emplace_back(path);
    }

    size_t extract_file_count = 0;

    for (const auto& [fst, snd] : directory_files) {
        if (is_stop_) {
            return extract_file_count;
        }

        ForwardList<TrackInfo> tracks;

        for (const auto & path : snd) {            
            try {
                TaglibMetadataReader reader;                
                tracks.push_front(reader.Extract(path));
                ++extract_file_count;
            }
            catch (...) { }
        }
        
        tracks.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit readFilePath(stringFormat("Extract directory {} size: {} completed.", String::ToString(fst), snd.size()));
        emit insertDatabase(tracks, playlist_id);
    }
    return extract_file_count;
}

void FileSystemWorker::onExtractFile(const QString& file_path, int32_t playlist_id) {
	const Stopwatch sw;

    constexpr QFlags<QDir::Filter> filter = QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs;
    QDirIterator itr(file_path, getTrackInfoFileNameFilter(), filter);

    Vector<QString> paths;
    paths.reserve(kReserveFilePathSize);

    while (itr.hasNext()) {
        if (is_stop_) {
            return;
        }
        auto path = toNativeSeparators(itr.next());
        paths.push_back(path);
    }

    if (paths.empty()) {
        paths.push_back(file_path);
    }

    emit readFileStart();

    XAMP_ON_SCOPE_EXIT(
        emit readFileProgress(100);
		emit readCompleted();
		XAMP_LOG_D(logger_, "Finish to read track info. ({} secs)", sw.ElapsedSeconds());
    );

    std::atomic<size_t> completed_work(0);

    auto [total_work, file_count_paths] = getPathSortByFileCount(paths, getTrackInfoFileNameFilter(), [this](auto total_file_count) {
        emit foundFileCount(total_file_count);
        });

    if (total_work == 0) {
        XAMP_LOG_DEBUG("Not found file: {}", String::ToString(file_path.toStdWString()));
        return;
    }

    const auto &file_name_filter = getTrackInfoFileNameFilter();

    Executor::ParallelFor(GetBackgroundThreadPool(), file_count_paths, [&](const auto& path_info) {
        if (is_stop_) {
            return;
        }
        XAMP_ON_SCOPE_EXIT(
            const auto value = completed_work.load();
            emit readFileProgress((value * 100) / total_work);
        );
        completed_work += scanPathFiles(file_name_filter, playlist_id, path_info.path);
        });
}

void FileSystemWorker::cancelRequested() {
    is_stop_ = true;
}
