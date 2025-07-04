﻿#include <widget/worker/filesystemservice.h>

#include <QDirIterator>
#include <execution>
#include <base/scopeguard.h>
#include <base/archivefile.h>
#include <widget/util/read_util.h>
#include <metadata/taglibmetareader.h>
#include <metadata/cueloader.h>

#include <widget/albumview.h>
#include <widget/database.h>
#include <widget/util/hash_util.h>
#include <widget/util/ui_util.h>

namespace {
	XAMP_DECLARE_LOG_NAME(FileSystemService);
	XAMP_DECLARE_LOG_NAME(FileSystemServiceThreadPool);

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

		Executor::ParallelForEach(thread_pool, path_infos, [&](auto& path_info)->void {
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
	: timer_(this)
	, database_(makeDatabaseConnection()) {
	(void)QObject::connect(&timer_, &QTimer::timeout,
		this, &FileSystemService::updateProgress);
	logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(FileSystemService));
	constexpr auto kThreadPoolSize = 8;
	thread_pool_ = ThreadPoolBuilder::MakeThreadPool(
		XAMP_LOG_NAME(FileSystemServiceThreadPool),
		kThreadPoolSize,
		1,
		ThreadPriority::PRIORITY_BACKGROUND);
}

FileSystemService::~FileSystemService() = default;

void FileSystemService::scanPathFiles(int32_t playlist_id, const QString& dir) {
	QDirIterator itr(dir,
		getTrackInfoFileNameFilter(), 
		QDir::NoDotAndDotDot | QDir::Files,
	    QDirIterator::Subdirectories);

	FloatMap<QString, std::vector<Path>> directory_files;
	std::vector<Path> cue_files;
	std::vector<Path> zip_files;

	directory_files.reserve(1024);
	cue_files.reserve(1024);
	zip_files.reserve(1024);

	XAMP_ON_SCOPE_EXIT(
		for (auto& pair_entry : directory_files) {
			pair_entry.second.clear();
			pair_entry.second.shrink_to_fit();
		}
	);

	auto collect_files = [this, &directory_files, &cue_files, &zip_files](const auto& dir) {
		const Path path(toNativeSeparators(dir).toStdWString());
		if (path.extension() == kCueFileExtension) {
			cue_files.push_back(path);
		}
		else if (path.extension() == kZipFileExtension) {
			zip_files.push_back(path);
		}
		else {
			const auto directory = QFileInfo(dir).dir().path();
			if (!directory_files.contains(directory)) {
				directory_files[directory].reserve(
					kReserveFilePathSize);
			}
			directory_files[directory].emplace_back(path);			
		}
		};

	while (itr.hasNext()) {
		if (is_stop_) {
			return;
		}
		collect_files(itr.next());
	}

	if (directory_files.empty()) {
		if (!QFileInfo(dir).isFile()) {
			XAMP_LOG_DEBUG("Not found file: {}",
				String::ToString(dir.toStdWString()));
			return;
		}
		collect_files(dir);
	}

	// directory_files 目的是為了將同一個檔案分類再一起,
	// 為了以下進行平行處理資料夾內的檔案, 並將解析後得結果進行track no排序.
	std::vector<std::forward_list<TrackInfo>> batch_track_infos;

	constexpr auto kMaxBatchTrackSize = 250UL;
	size_t track_count = 0;

	Executor::ParallelForEach(thread_pool_,
		zip_files, [&](auto& files_path, const auto& stop_token) {
			ArchiveFile archive_file;
			auto result = archive_file.Open(files_path);
			if (!result) {
				XAMP_LOG_D(logger_, result.error());
				return;
			}
			total_work_ += result.value().size();
		}
	);

	Executor::ParallelForEach(thread_pool_,
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
			reader->Open(path);			
			auto track_info = reader->Extract();
			if (track_info) {
				tracks.push_front(track_info.value());
			}
			++completed_work_;
			++track_count;
			updateProgress();
		}

		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
		});

		std::scoped_lock lock(batch_mutex_);
		batch_track_infos.emplace_back(std::move(tracks));

		if (track_count > kMaxBatchTrackSize) {
			emit readFilePath(
				qFormat("Scan directory %1 size:%2 completed.")
				.arg(path_info.first)
				.arg(path_info.second.size()));
 			emit batchInsertDatabase(batch_track_infos, playlist_id);
			batch_track_infos.clear();
			track_count = 0;
		}
	}, stop_source_.get_token());

	if (!batch_track_infos.empty()) {
		emit readFilePath(qFormat("Scan directory %1 size:%2 completed.")
			.arg(directory_files.begin()->first)
			.arg(directory_files.begin()->second.size()));
		emit batchInsertDatabase(batch_track_infos, playlist_id);
	}	

	Executor::ParallelForEach(thread_pool_,
		zip_files, [&](auto& files_path, const auto& stop_token) {
		if (stop_token.stop_requested()) {
			return;
		}
		
		ArchiveFile archive_file;
		auto result = archive_file.Open(files_path);
		if (!result) {
			XAMP_LOG_D(logger_, result.error());
			return;
		}

		std::forward_list<TrackInfo> tracks;
		for (const auto& entry_name : result.value()) {
			auto entry = archive_file.OpenEntry(entry_name);

			try {
				if (entry) {
					TaglibMetadataReader reader;
					reader.Open(std::move(entry.value()));
					auto track_info = reader.Extract();
					if (track_info) {
						tracks.push_front(track_info.value());
					}
				}
			}
			catch (const std::exception& e) {
				XAMP_LOG_DEBUG("{}", e.what());
			}			
			++completed_work_;
			updateProgress();
		}		

		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
			});
		if (!tracks.empty()) {
			emit insertDatabase(tracks, playlist_id);
		}
		});

	Executor::ParallelForEach(thread_pool_,
		cue_files, [&](auto& files_path, const auto& stop_token) {
		if (stop_token.stop_requested()) {
			return;
		}

		std::forward_list<TrackInfo> tracks;
		{
			std::scoped_lock lock(batch_mutex_);
			CueLoader loader;
			auto track_infos = loader.Load(files_path);
			if (track_infos) {
				for (auto& track : track_infos.value()) {
					tracks.push_front(track);
				}
			}
		}
		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
			});
		if (!tracks.empty()) {
			emit insertDatabase(tracks, playlist_id);
		}
		});
}

void FileSystemService::scanReplayGain(const QList<PlayListEntity>& entities) {
	FastMutex mutex;

	total_work_ = 0;
	completed_work_ = 0;
	last_completed_work_ = 0;
	total_time_elapsed_.Reset();
	update_ui_elapsed_.Reset();
	timer_.start(std::chrono::seconds(1));

	HashMap<QString, QList<PlayListEntity>> album_entities;
	for (const auto& entity : entities) {
		album_entities[entity.album].push_back(entity);
		total_work_++;
	}	

	XAMP_ON_SCOPE_EXIT(
		timer_.stop();
		emit readFileProgress(100);
		emit readCompleted();
		XAMP_LOG_D(logger_, "Finish to scan replay gain. ({} secs)",
		total_time_elapsed_.ElapsedSeconds());
	);

	auto progress_cb = [this](int32_t progress) {
		return true;
		};

	for (auto& album : album_entities) {
		std::vector<Ebur128Scanner> scanners;
		scanners.resize(album.second.size());

		auto stop_token = stop_source_.get_token();
		Executor::ParallelForEach(thread_pool_, 0, album.second.size(),
			[&](auto index) {								
				if (!stop_token.stop_requested()) {
					std::scoped_lock lock(mutex);
					try {
						scanners[index] = readFileLoudness(
							album.second[index].file_path.toStdWString(),
							progress_cb);
					}
					catch (const Exception& e) {
						XAMP_LOG_ERROR(e.GetErrorMessage());
					}
					++completed_work_;
					emit readFilePath(
						qFormat("Scan directory %1 completed.")
						.arg(album.second[index].file_path));
					updateProgress();
				}				
			});

		if (stop_token.stop_requested()) {
			return;
		}

		try {
			double album_peak = std::numeric_limits<double>::min();
			for (auto i = 0; i < album.second.size(); ++i) {
				if (!scanners[i].IsValid()) {
					XAMP_LOG_DEBUG("In-completed read file replay gain");
					continue;
				}
				album.second[i].replay_gain.value().track_peak = scanners[i].GetTruePeek();
				album.second[i].replay_gain.value().track_gain = scanners[i].GetLoudness();
				album.second[i].replay_gain.value().ref_loudness = Ebur128Scanner::kReferenceLoudness;
				album_peak = (std::max)(album_peak, scanners[i].GetTruePeek());
			}

			auto album_replay_gain = Ebur128Scanner::GetMultipleLoudness(scanners);
			scanners.clear(); // !!!!! Key point

			for (auto& entity : album.second) {
				entity.replay_gain.value().album_peak = album_peak;
				entity.replay_gain.value().album_gain = album_replay_gain;
				try {
					auto writer = MakeMetadataWriter();
					writer->Open(entity.file_path.toStdWString());
					writer->WriteReplayGain(entity.replay_gain.value());
				}
				catch (const Exception& e) {
					XAMP_LOG_ERROR(e.GetErrorMessage());
				}
			}

			emit scanReplayGainCompleted(album.second);
		}
		catch (const Exception& e) {
			XAMP_LOG_ERROR(e.GetErrorMessage());
			emit scanReplayGainError();
		}
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
		XAMP_LOG_D(logger_, "Finish to scan track info. ({} secs)", 
			total_time_elapsed_.ElapsedSeconds());
	);

	if (total_work == 0) {
		XAMP_LOG_DEBUG("Not found file: {}",
			String::ToString(file_path.toStdWString()));
		return;
	}

	playlist_id_ = playlist_id;
	completed_work_ = 0;
	last_completed_work_ = 0;
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
			XAMP_LOG_DEBUG("Failed to scan file:{} ({})", 
				String::ToString(path_info.path.toStdWString()), 
				String::LocaleStringToUTF8(e.what()));
		}		
	}
}

void FileSystemService::updateProgress() {
	if (is_stop_ || stop_source_.stop_requested()) {
		return;
	}

	if (update_ui_elapsed_.ElapsedSeconds() < 1.0) {
		return;
	}

	const auto completed_work = completed_work_.load();
	emit readFileProgress((completed_work * 100) / total_work_);
	const auto elapsed_time = total_time_elapsed_.ElapsedSeconds();

	size_t diff_work = completed_work - last_completed_work_;
	if (elapsed_time > 0.0) {
		double files_per_second = static_cast<double>(diff_work) / elapsed_time;
		XAMP_LOG_DEBUG("Speed: {:.2f} files/sec", files_per_second);
	}

	const double remaining_time = (total_work_ > completed_work)
		? (static_cast<double>(elapsed_time)
			/ static_cast<double>(completed_work)) *
		(total_work_ - completed_work)
		: 0.0;
	emit remainingTimeEstimation(total_work_,
		completed_work,
		static_cast<int32_t>(remaining_time));

	last_completed_work_ = completed_work;
	update_ui_elapsed_.Reset();
}

void FileSystemService::cancelRequested() {
	is_stop_ = true;
	stop_source_.request_stop();
}
