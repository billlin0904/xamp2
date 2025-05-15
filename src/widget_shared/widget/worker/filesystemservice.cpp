#include <widget/worker/filesystemservice.h>

#include <QDirIterator>
#include <execution>
#include <base/scopeguard.h>

#include <metadata/taglibmetareader.h>
#include <metadata/cueloader.h>

#include <widget/albumview.h>
#include <widget/util/ui_util.h>

namespace {
	XAMP_DECLARE_LOG_NAME(FileSystemService);

	struct PathInfo {
		size_t file_count;
		size_t depth;
		QString path;
	};

	std::pair<size_t, std::vector<PathInfo>> getPathSortByFileCount(
		const std::shared_ptr<IThreadPoolExecutor>& thread_pool,
		const std::stop_token &stop_token,
		const std::vector<QString>& paths,
		const QStringList& file_name_filters,
		std::function<void(size_t)>&& action) {
		std::atomic<size_t> total_file_count = 0;
		std::vector<PathInfo> path_infos;
		path_infos.reserve(paths.size());

		// Calculate file counts and depths in parallel
		for (const auto& path : paths) {
			path_infos.push_back({0, 0, path});
		}

		Executor::ParallelFor(thread_pool.get(), path_infos, [&](auto& path_info)->void {
			path_info.file_count = getFileCount(path_info.path, file_name_filters);
			path_info.depth = path_info.path.count("/"_str);
			total_file_count += path_info.file_count;
			action(total_file_count);
		}, stop_token);

		std::erase_if(path_infos,
		              [](const auto& info) { return info.file_count == 0; });

		if (total_file_count == 0) {
			return std::make_pair(total_file_count.load(), path_infos);
		}

        #ifdef Q_OS_WIN
		std::sort(std::execution::par_unseq, path_infos.begin(), path_infos.end(),
			[](const auto& p1, const auto& p2) {
			if (p1.file_count != p2.file_count) {
				return p1.file_count > p2.file_count;
			}
			return p1.depth < p2.depth;
		});
		#else
		std::sort(path_infos.begin(), path_infos.end(), [](const auto& p1, const auto& p2) {
			if (p1.file_count != p2.file_count) {
				return p1.file_count > p2.file_count;
	}
			return p1.depth < p2.depth;
		});
        #endif
		return std::make_pair(total_file_count.load(), path_infos);
	}
}

FileSystemService::FileSystemService()
	: timer_(this) {
	(void)QObject::connect(&timer_, &QTimer::timeout,
		this, &FileSystemService::updateProgress);
	logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(FileSystemService));
	constexpr auto kThreadPoolSize = 8;
	thread_pool_ = ThreadPoolBuilder::MakeThreadPool(
		XAMP_LOG_NAME(FileSystemService),
		kThreadPoolSize,
		1,
		ThreadPriority::PRIORITY_BACKGROUND);
}

FileSystemService::~FileSystemService() {
}

void FileSystemService::scanPathFiles(int32_t playlist_id, const QString& dir) {
	QDirIterator itr(dir,
		getTrackInfoFileNameFilter(), 
		QDir::NoDotAndDotDot | QDir::Files,
	    QDirIterator::Subdirectories);

	FloatMap<QString, std::vector<Path>> directory_files;

	XAMP_ON_SCOPE_EXIT(
		for (auto& files : directory_files) {
			files.second.clear();
			files.second.shrink_to_fit();
		}
	);

	auto database_pool = getPooledDatabase(1);
	auto database_ptr = database_pool->Acquire();
	DatabaseFacade facade(nullptr, database_ptr.get());

	// Note: CueLoader has thread safe issue so we need to process not in parallel.
	auto process_cue_file = [this, &facade](const auto &path, auto playlist_id) {
		try {
			std::forward_list<TrackInfo> tracks;
			CueLoader loader;
			for (auto& track : loader.Load(path)) {
				tracks.push_front(track);
			}
			tracks.sort([](const auto& first, const auto& last) {
				return first.track < last.track;
				});
			try {
				facade.insertTrackInfo(tracks, playlist_id, StoreType::LOCAL_STORE);
			}
			catch (const Exception& e) {
				XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
			}
			catch (const std::exception& e) {
				XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
			}
			catch (...) {
			}
			emit insertDatabase(tracks, playlist_id);
		}
		catch (const std::exception &e) {
			XAMP_LOG_DEBUG("Failed to extract cue file: {}", e.what());
		}
		catch (...) {	
		}			
	};

	auto process_file = [this, &directory_files, 
		playlist_id, 
		process_cue_file](const auto& dir) {
		const auto next_path = toNativeSeparators(dir);
		const auto path = next_path.toStdWString();
		const Path test_path(path);
		if (test_path.extension() != kCueFileExtension) {
			const auto directory = QFileInfo(dir).dir().path();
			if (!directory_files.contains(directory)) {
				directory_files[directory].reserve(
					kReserveFilePathSize);
			}
			directory_files[directory].emplace_back(path);
		}
		else {
			process_cue_file(test_path, playlist_id);
		}
		};

	while (itr.hasNext()) {
		if (is_stop_) {
			return;
		}
		process_file(itr.next());
	}

	if (directory_files.empty()) {
		if (!QFileInfo(dir).isFile()) {
			XAMP_LOG_DEBUG("Not found file: {}",
				String::ToString(dir.toStdWString()));
			return;
		}
		process_file(dir);
	}

	// directory_files 目的是為了將同一個檔案分類再一起,
	// 為了以下進行平行處理資料夾內的檔案, 並將解析後得結果進行track no排序.

	FastMutex batch_mutex;
	std::vector<std::forward_list<TrackInfo>> batch_track_infos;
#ifdef _DEBUG
	constexpr auto kMaxBatchSize = 100UL;
#else
	constexpr auto kMaxBatchSize = 500UL;
#endif

	batch_track_infos.reserve(kMaxBatchSize);

	Executor::ParallelFor(thread_pool_.get(),
		directory_files, [&](auto& path_info, const auto& stop_token) {
		if (is_stop_) {
			return;
		}

		std::forward_list<TrackInfo> tracks;
		auto reader = MakeMetadataReader();

		for (const auto& path : path_info.second) {
			if (stop_token.stop_requested()) {
				return;
			}

			try {
				reader->Open(path);
				tracks.push_front(reader->Extract());
			}
			catch (const std::exception &e) {
				XAMP_LOG_DEBUG("Failed to extract file: {}", e.what());
			}
			catch (...) {
			}
			++completed_work_;
			updateProgress();
		}

		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
		});

		std::scoped_lock lock(batch_mutex);
		batch_track_infos.emplace_back(std::move(tracks));

		if (batch_track_infos.size() > kMaxBatchSize) {
			emit readFilePath(
				qFormat("Extract directory %1 size:%2 completed.")
				.arg(path_info.first)
				.arg(path_info.second.size()));
			try {
				facade.insertMultipleTrackInfo(batch_track_infos, playlist_id, StoreType::LOCAL_STORE);
			}
			catch (const Exception& e) {
				XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
			}
			catch (const std::exception& e) {
				XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
			}
			catch (...) {
			}
			
 			emit batchInsertDatabase(batch_track_infos, playlist_id);
			batch_track_infos.clear();
		}		
	}, stop_source_.get_token());

	if (!batch_track_infos.empty()) {
		emit readFilePath(qFormat("Extract directory %1 size:%2 completed.")
			.arg(directory_files.begin()->first)
			.arg(directory_files.begin()->second.size()));
		try {
			facade.insertMultipleTrackInfo(batch_track_infos, playlist_id, StoreType::LOCAL_STORE);
		}
		catch (const Exception& e) {
			XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
		}
		catch (const std::exception& e) {
			XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
		}
		catch (...) {
		}
		emit batchInsertDatabase(batch_track_infos, playlist_id);
	}
}

void FileSystemService::onExtractFile(const QString& file_path,
	int32_t playlist_id) {
	is_stop_ = false;
	stop_source_ = std::stop_source();

	QDirIterator itr(file_path,
		getTrackInfoFileNameFilter(), 
		QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs);

	std::vector<QString> paths;
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

	auto [total_work, file_count_paths] =
		getPathSortByFileCount(thread_pool_, stop_source_.get_token(),
			paths,
			getTrackInfoFileNameFilter(),
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
		XAMP_LOG_D(logger_, "Finish to read track info. ({} secs)", 
			total_time_elapsed_.ElapsedSeconds());
	);

	if (total_work == 0) {
		XAMP_LOG_DEBUG("Not found file: {}",
			String::ToString(file_path.toStdWString()));
		return;
	}

	completed_work_ = 0;
	total_work_ = total_work;
	total_time_elapsed_.Reset();
	update_ui_elapsed_.Reset();

	timer_.start(std::chrono::seconds(1));

	for (const auto& path_info : file_count_paths) {
		if (is_stop_) {
			return;
		}

		try {
			scanPathFiles(playlist_id, path_info.path);
		}
		catch (const std::exception &e) {
			XAMP_LOG_DEBUG("Failed to extract file:{} ({})", 
				String::ToString(path_info.path.toStdWString()), 
				String::LocaleStringToUTF8(e.what()));
		}		
	}
}

void FileSystemService::updateProgress() {
	if (is_stop_ || stop_source_.stop_requested()) {
		return;
	}

	if (update_ui_elapsed_.ElapsedSeconds() < 3.0) {
		return;
	}

	const auto completed_work = completed_work_.load();
	emit readFileProgress((completed_work * 100) / total_work_);
	const auto elapsed_time = total_time_elapsed_.ElapsedSeconds();
	const double remaining_time = (total_work_ > completed_work)
		? (static_cast<double>(elapsed_time)
			/ static_cast<double>(completed_work)) *
		(total_work_ - completed_work)
		: 0.0;
	emit remainingTimeEstimation(total_work_,
		completed_work,
		static_cast<int32_t>(remaining_time));
	update_ui_elapsed_.Reset();
}

void FileSystemService::cancelRequested() {
	is_stop_ = true;
	stop_source_.request_stop();
}
