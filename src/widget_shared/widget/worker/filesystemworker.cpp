#include <widget/worker/filesystemworker.h>

#include <QDirIterator>

#include <base/scopeguard.h>

#include <metadata/taglibmetareader.h>

#include <widget/albumview.h>
#include <widget/util/ui_utilts.h>

XAMP_DECLARE_LOG_NAME(FileSystemWorker);
XAMP_DECLARE_LOG_NAME(ExtractFileWorker);

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
		for (const auto& path : paths) {
			path_infos.push_back({0, 0, path});
		}

		Executor::ParallelFor(GetBackgroundThreadPool(), path_infos, [&](auto& path_info) {
			path_info.file_count = getFileCount(path_info.path, file_name_filters);
			path_info.depth = path_info.path.count(qTEXT("/"));
			total_file_count += path_info.file_count;
			action(total_file_count);
		});

		std::erase_if(path_infos,
		              [](const auto& info) { return info.file_count == 0; });

		if (total_file_count == 0) {
			return std::make_pair(total_file_count.load(), path_infos);
		}
		std::sort(path_infos.begin(), path_infos.end(), [](const auto& p1, const auto& p2) {
			if (p1.file_count != p2.file_count) {
				return p1.file_count > p2.file_count;
			}
			return p1.depth < p2.depth;
		});
		return std::make_pair(total_file_count.load(), path_infos);
	}
}

FileSystemWorker::FileSystemWorker()
	: watcher_(this)
	, timer_(this) {
	(void)QObject::connect(&timer_, &QTimer::timeout, this, &FileSystemWorker::updateProgress);
	logger_ = XampLoggerFactory.GetLogger(kFileSystemWorkerLoggerName);
	GetBackgroundThreadPool();
	(void)QObject::connect(&watcher_,
	                       &FileSystemWatcher::directoryChanged,
	                       this,
	                       [this](const auto& dir) {
		                       onExtractFile(dir, -1);
	                       });
}

FileSystemWorker::~FileSystemWorker() {
	GetBackgroundThreadPool().Stop();
}

void FileSystemWorker::onSetWatchDirectory(const QString& dir) {
	watcher_.addPath(dir);
}

void FileSystemWorker::scanPathFiles(AlignPtr<IThreadPoolExecutor>& thread_pool, int32_t playlist_id, const QString& dir) {
	QDirIterator itr(dir, getTrackInfoFileNameFilter(), QDir::NoDotAndDotDot | QDir::Files,
	                 QDirIterator::Subdirectories);
	FloatMap<std::wstring, Vector<Path>> directory_files;

	XAMP_ON_SCOPE_EXIT(
		for (auto& files : directory_files) {
		files.second.clear();
		files.second.shrink_to_fit();
		}
	);

	while (itr.hasNext()) {
		if (is_stop_) {
			return;
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
			return;
		}
		const auto next_path = toNativeSeparators(dir);
		const auto path = next_path.toStdWString();
		const auto directory = QFileInfo(dir).dir().path().toStdWString();
		directory_files[directory].emplace_back(path);
	}

	Executor::ParallelFor(*thread_pool, directory_files, [&](const auto& path_info) {
		if (is_stop_) {
			return;
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ForwardList<TrackInfo> tracks;

		for (const auto& path : path_info.second) {
			try {
				TaglibMetadataReader reader;
				tracks.push_front(reader.Extract(path));
				++completed_work_;
				updateProgress();
			}
			catch (const std::exception &e) {
				XAMP_LOG_DEBUG("Failed to extract file:{}", path.string());
			}
		}

		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
		});
		emit readFilePath(stringFormat("Extract directory {} size:{} completed.",
			String::ToString(path_info.first),
			path_info.second.size()));
		emit insertDatabase(tracks, playlist_id);
	});
}

void FileSystemWorker::onExtractFile(const QString& file_path, int32_t playlist_id) {
	is_stop_ = false;

	auto extract_file_thread_pool = MakeThreadPoolExecutor(kExtractFileWorkerLoggerName, ThreadPriority::BACKGROUND);

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
	std::atomic<size_t> completed_work(0);

	auto [total_work, file_count_paths] =
		getPathSortByFileCount(paths, getTrackInfoFileNameFilter(),
		                       [this](auto total_file_count) {
			                       emit foundFileCount(total_file_count);
		                       });

	XAMP_ON_SCOPE_EXIT(
		timer_.stop();
		paths.clear();
		paths.shrink_to_fit();

		file_count_paths.clear();
		file_count_paths.shrink_to_fit();

		emit readFileProgress(100);
		emit readCompleted();
		XAMP_LOG_D(logger_, "Finish to read track info. ({} secs)", total_time_elapsed_.ElapsedSeconds());
	);

	if (total_work == 0) {
		XAMP_LOG_DEBUG("Not found file: {}", String::ToString(file_path.toStdWString()));
		return;
	}

	completed_work_ = 0;
	total_work_ = total_work;
	total_time_elapsed_.Reset();

	timer_.start(std::chrono::seconds(1));

	for (const auto& path_info : file_count_paths) {
		if (is_stop_) {
			return;
		}
		scanPathFiles(extract_file_thread_pool, playlist_id, path_info.path);
	}
}

void FileSystemWorker::updateProgress() {
	const auto completed_work = completed_work_.load();
	emit readFileProgress((completed_work * 100) / total_work_);
	if (update_elapsed_.ElapsedSeconds() > 1) {
		const auto elapsed_time = total_time_elapsed_.ElapsedSeconds();
		const double remaining_time = (total_work_ > completed_work)
			                              ? (static_cast<double>(elapsed_time) / static_cast<double>(completed_work)) *
			                              (total_work_ - completed_work)
			                              : 0.0;
		emit remainingTimeEstimation(total_work_, completed_work, static_cast<int32_t>(remaining_time));
		update_elapsed_.Reset();
	}
}

void FileSystemWorker::cancelRequested() {
	is_stop_ = true;
}
