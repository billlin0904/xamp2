#include <widget/worker/filesystemservice.h>

#include <iterator>

#include <base/scopeguard.h>
#include <metadata/metadatalibraryscanner.h>

#include <widget/albumview.h>
#include <widget/database.h>
#include <widget/util/ui_util.h>

namespace {
	XAMP_DECLARE_LOG_NAME(FileSystemService);

	size_t CountTracks(const std::forward_list<TrackInfo>& tracks) {
		return static_cast<size_t>(std::distance(tracks.begin(), tracks.end()));
	}

	size_t CountTrackBatches(const std::vector<std::forward_list<TrackInfo>>& batches) {
		size_t track_count = 0;
		for (const auto& tracks : batches) {
			track_count += CountTracks(tracks);
		}
		return track_count;
	}
}

FileSystemService::FileSystemService()
	: timer_(this)
	, database_(makeDatabaseConnection()) {
	(void)QObject::connect(&timer_, &QTimer::timeout,
		this, &FileSystemService::updateProgress);
	logger_ = XAMP_LOG_CREATE_LOGGER(FileSystemService);
}

void FileSystemService::setScannerThreadPool(std::shared_ptr<IThreadPoolExecutor> scanner_thread_pool) {
    XAMP_ASSERT(scanner_thread_pool);
    thread_pool_ = std::move(scanner_thread_pool);
}

FileSystemService::~FileSystemService() = default;

void FileSystemService::onExtractFile(const QString& file_path,
	int32_t playlist_id) {
	is_stop_ = false;
	stop_source_ = std::stop_source();

	emit readFileStart();

	XAMP_ON_SCOPE_EXIT(
		timer_.stop();
		emit readFileProgress(100);
		emit readCompleted();
		XAMP_LOG_D(logger_, "Finish to scan track info. ({} secs)", 
			total_time_elapsed_.ElapsedSeconds());
	);

	playlist_id_ = playlist_id;
	completed_work_ = 0;
	last_completed_work_ = 0;
	total_work_ = 0;
	total_time_elapsed_.Reset();
	update_ui_elapsed_.Reset();

	timer_.start(std::chrono::seconds(1));

	try {
		MetadataLibraryScanner scanner(thread_pool_);
		MetadataScanCallbacks callbacks;
		callbacks.on_found_file_count = [this](size_t total_file_count) {
			total_work_ = total_file_count;
			emit foundFileCount(total_file_count);
			};
		callbacks.on_read_path = [this](const Path& path, size_t size) {
			emit readFilePath(
				qFormat("Scan directory %1 size:%2 completed.")
				.arg(QString::fromStdWString(path.wstring()))
				.arg(size));
			};
		callbacks.on_progress = [this](MetadataScanProgress progress) {
			total_work_ = progress.total_work;
			completed_work_ = progress.completed_work;
			updateProgress();
			};
		callbacks.on_batch_tracks = [this, playlist_id](auto tracks) {
			const auto track_count = CountTrackBatches(tracks);
			XAMP_LOG_D(logger_,
				"Metadata scan batch ready playlist:{} batches:{} tracks:{} elapsed:{:.3f}s",
				playlist_id,
				tracks.size(),
				track_count,
				total_time_elapsed_.ElapsedSeconds());
			emit batchInsertDatabase(tracks, playlist_id);
			};
		callbacks.on_tracks = [this, playlist_id](auto tracks) {
			const auto track_count = CountTracks(tracks);
			XAMP_LOG_D(logger_,
				"Metadata scan tracks ready playlist:{} tracks:{} elapsed:{:.3f}s",
				playlist_id,
				track_count,
				total_time_elapsed_.ElapsedSeconds());
			emit insertDatabase(tracks, playlist_id);
			};

		const Path root_path(toNativeSeparators(file_path).toStdWString());
		const auto result = scanner.Scan(root_path,
			stop_source_.get_token(),
			callbacks);

		if (result.total_work == 0) {
			XAMP_LOG_DEBUG("Not found file: {}",
				String::ToString(file_path.toStdWString()));
		}
	}
	catch (const std::exception& e) {
		XAMP_LOG_DEBUG("Failed to scan file:{} ({})",
			String::ToString(file_path.toStdWString()),
			String::LocaleStringToUTF8(e.what()));
	}
}

void FileSystemService::updateProgress() {
	if (is_stop_ || stop_source_.stop_requested()) {
		return;
	}

	std::scoped_lock lock(progress_mutex_);
	if (update_ui_elapsed_.ElapsedSeconds() < 1.0) {
		return;
	}

	const auto completed_work = completed_work_.load();
	const auto total_work = total_work_.load();
	if (total_work == 0) {
		return;
	}

	emit readFileProgress(static_cast<int32_t>((completed_work * 100) / total_work));
	const auto elapsed_time = total_time_elapsed_.ElapsedSeconds();

	size_t diff_work = completed_work - last_completed_work_;
	if (elapsed_time > 0.0) {
		double files_per_second = static_cast<double>(diff_work) / elapsed_time;
		XAMP_LOG_DEBUG("Speed: {:.2f} files/sec", files_per_second);
	}

	const double remaining_time = (total_work > completed_work)
		? (static_cast<double>(elapsed_time)
			/ static_cast<double>(completed_work)) *
		(total_work - completed_work)
		: 0.0;
	emit remainingTimeEstimation(total_work,
		completed_work,
		static_cast<int32_t>(remaining_time));

	last_completed_work_ = completed_work;
	update_ui_elapsed_.Reset();
}

void FileSystemService::cancelRequested() {
	is_stop_ = true;
	stop_source_.request_stop();
}
