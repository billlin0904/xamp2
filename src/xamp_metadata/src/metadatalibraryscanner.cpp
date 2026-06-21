//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <metadata/metadatalibraryscanner.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <functional>
#include <iterator>
#include <mutex>
#include <optional>
#include <utility>

#include <base/executor.h>
#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/stopwatch.h>
#include <base/threadpoolbuilder.h>
#include <base/scopeguard.h>

#include <metadata/api.h>
#include <metadata/cuefilereader.h>

XAMP_METADATA_NAMESPACE_BEGIN

namespace {
	XAMP_DECLARE_LOG_NAME(MetadataLibraryScanner);

	constexpr auto kCueFileExtension = ".cue";

	struct ScanFiles final {
		HashMap<Path, std::vector<Path>> directory_files;
		std::vector<Path> cue_files;
	};

	std::string pathToUtf8(const Path& path) {
		return String::toString(path.wstring());
	}

	std::string normalizeExtension(const Path& path) {
		return String::toLower(String::toString(path.extension().wstring()));
	}

	bool isSupportedTrackFile(const Path& path) {
		return getSupportFileExtensions().contains(normalizeExtension(path));
	}

	void collectFile(const Path& path, ScanFiles& files) {
		const auto extension = normalizeExtension(path);
		if (extension == kCueFileExtension) {
			files.cue_files.push_back(path);
		}
		else if (isSupportedTrackFile(path)) {
			auto parent_path = path.parent_path();
			auto& directory = files.directory_files[parent_path];
			if (directory.empty()) {
				directory.reserve(1024);
			}
			directory.push_back(path);
		}
	}

	void collectFiles(const Path& root_path,
		ScanFiles& files,
		const MetadataScanOptions& options,
		const std::stop_token& stop_token) {
		std::error_code ec;
		if (Fs::is_regular_file(root_path, ec)) {
			collectFile(root_path, files);
			return;
		}

		if (!Fs::is_directory(root_path, ec)) {
			return;
		}

		auto collect_entry = [&](const DirectoryEntry& entry) {
			if (entry.is_regular_file(ec)) {
				collectFile(entry.path(), files);
			}
			if (ec) {
				XAMP_LOG_DEBUG("Failed to read path: {} ({})",
					pathToUtf8(entry.path()),
					ec.message());
				ec.clear();
			}
			};

		if (!options.recursive) {
			for (DirectoryIterator it(root_path, kIteratorOptions, ec), end; it != end && !stop_token.stop_requested();) {
				collect_entry(*it);
				it.increment(ec);
				if (ec) {
					XAMP_LOG_DEBUG("Failed to iterate path: {} ({})",
						pathToUtf8(root_path),
						ec.message());
					ec.clear();
				}
			}
			return;
		}

		for (RecursiveDirectoryIterator it(root_path, kIteratorOptions, ec), end; it != end && !stop_token.stop_requested();) {
			const auto& entry = *it;
			collect_entry(entry);
			it.increment(ec);
			if (ec) {
				XAMP_LOG_DEBUG("Failed to iterate path: {} ({})",
					pathToUtf8(root_path),
					ec.message());
				ec.clear();
			}
		}
	}

	size_t countScanFiles(const ScanFiles& files) {
		size_t total_work = files.cue_files.size();
		for (const auto& [directory, paths] : files.directory_files) {
			total_work += paths.size();
		}
		return total_work;
	}

	void sortTracks(std::forward_list<TrackInfo>& tracks) {
		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
			});
	}

	size_t countTracks(const std::forward_list<TrackInfo>& tracks) {
		return static_cast<size_t>(std::distance(tracks.begin(), tracks.end()));
	}

	size_t countTrackBatches(const std::vector<std::forward_list<TrackInfo>>& batches) {
		size_t track_count = 0;
		for (const auto& tracks : batches) {
			track_count += countTracks(tracks);
		}
		return track_count;
	}

	template <typename Callback, typename... Args>
	void invokeCallback(const Callback& callback, Args&&... args) {
		if (callback) {
			std::invoke(callback, std::forward<Args>(args)...);
		}
	}
}

MetadataLibraryScanner::MetadataLibraryScanner(std::shared_ptr<IThreadPool> thread_pool)
	: thread_pool_(std::move(thread_pool)) {
	XAMP_ENSURES(thread_pool_ != nullptr);
}

MetadataScanProgress MetadataLibraryScanner::scan(const Path& root_path,
	const std::stop_token& stop_token,
	const MetadataScanCallbacks& callbacks,
	const MetadataScanOptions& options) {
	Stopwatch total_elapsed;
	Stopwatch stage_elapsed;

	ScanFiles files;
	collectFiles(root_path, files, options, stop_token);
	const auto collect_seconds = stage_elapsed.elapsedSeconds();

	stage_elapsed.reset();
	const auto total_work = countScanFiles(files);
	const auto count_seconds = stage_elapsed.elapsedSeconds();
	invokeCallback(callbacks.on_found_file_count, total_work);

	XAMP_LOG_DEBUG("Metadata scan prepare path:{} total:{} directories:{} cues:{} collect:{:.3f}s count:{:.3f}s",
		pathToUtf8(root_path),
		total_work,
		files.directory_files.size(),
		files.cue_files.size(),
		collect_seconds,
		count_seconds);

	MetadataScanProgress progress;
	progress.total_work = total_work;
	if (total_work == 0 || stop_token.stop_requested()) {
		XAMP_LOG_DEBUG("Metadata scan completed path:{} total:{} completed:{} elapsed:{:.3f}s",
			pathToUtf8(root_path),
			total_work,
			progress.completed_work,
			total_elapsed.elapsedSeconds());
		return progress;
	}

	std::atomic<size_t> completed_work = 0;
	auto notify_progress = [&]() {
		const auto completed = ++completed_work;
		MetadataScanProgress progress;
		progress.total_work = total_work;
		progress.completed_work = completed;
		invokeCallback(callbacks.on_progress, progress);
		};

	FastMutex batch_mutex;
	std::vector<std::forward_list<TrackInfo>> batch_track_infos;
	batch_track_infos.reserve(options.batch_size);
	size_t batch_track_count = 0;

	auto flush_batch = [&](const Path& path, size_t path_size, bool force) {
		std::vector<std::forward_list<TrackInfo>> batch;
		{
			std::scoped_lock lock(batch_mutex);
			if (batch_track_infos.empty()) {
				return;
			}
			if (!force && batch_track_count < options.batch_size) {
				return;
			}
			batch = std::move(batch_track_infos);
			batch_track_infos.clear();
			batch_track_infos.reserve(options.batch_size);
			batch_track_count = 0;
		}

		const auto batch_size = batch.size();
		const auto track_count = countTrackBatches(batch);
		XAMP_LOG_DEBUG("Metadata scan emit batch path:{} directories:{} tracks:{} elapsed:{:.3f}s",
			pathToUtf8(path),
			batch_size,
			track_count,
			total_elapsed.elapsedSeconds());

		invokeCallback(callbacks.on_read_path, path, path_size);
		invokeCallback(callbacks.on_batch_tracks, std::move(batch));
		};

	constexpr auto kIOThreadCount = 8;
	constexpr auto kIOBulkSize = 2;
	auto io_thread_pool = ThreadPoolBuilder::makeThreadPool("IO ThreadPool",
		kIOThreadCount,
		kIOBulkSize,
		ThreadPriority::PRIORITY_BACKGROUND);

	stage_elapsed.reset();
	Executor::parallelFor(thread_pool_,
		files.directory_files,
		[&](auto& path_info, const auto& token) {
			if (stop_token.stop_requested() || token.stop_requested()) {
				return;
			}

			std::forward_list<TrackInfo> tracks;
			auto track_results = Executor::parallelFor(io_thread_pool,
				path_info.second,
				[&](const auto& path, const auto& io_stop_token) -> std::optional<TrackInfo> {
					if (stop_token.stop_requested() || token.stop_requested() || io_stop_token.stop_requested()) {
						return std::nullopt;
					}

					XAMP_ON_SCOPE_EXIT(
						notify_progress()
					);

					try {
						auto reader = makeMetadataReader();
						reader->open(path);
						auto track_info = reader->extract();
						if (track_info) {
							return std::move(track_info.value());
						}
					}
					catch (const std::exception& e) {
						XAMP_LOG_DEBUG("Failed to read metadata: {} ({})",
							pathToUtf8(path),
							e.what());
					}
					return std::nullopt;
				}, stop_token);

			size_t local_track_count = 0;
			for (auto& result : track_results) {
				if (result.has_value() && result->has_value()) {
					tracks.push_front(std::move(result->value()));
					++local_track_count;
				}
			}

			sortTracks(tracks);
			{
				std::scoped_lock lock(batch_mutex);
				if (!tracks.empty()) {
					batch_track_infos.emplace_back(std::move(tracks));
					batch_track_count += local_track_count;
				}
			}
			flush_batch(path_info.first, path_info.second.size(), false);
		},
		stop_token);
	XAMP_LOG_DEBUG("Metadata scan read directories count:{} elapsed:{:.3f}s total_elapsed:{:.3f}s",
		files.directory_files.size(),
		stage_elapsed.elapsedSeconds(),
		total_elapsed.elapsedSeconds());

	flush_batch(root_path, 0, true);


	FastMutex cue_mutex;
	stage_elapsed.reset();
	Executor::parallelFor(thread_pool_,
		files.cue_files,
		[&](auto& cue_path, const auto& token) {
			if (stop_token.stop_requested() || token.stop_requested()) {
				return;
			}

			std::forward_list<TrackInfo> tracks;
			try
			{
				std::scoped_lock lock(cue_mutex);
				CueLoader loader;
				auto track_infos = loader.load(cue_path);
				if (track_infos) {
					for (auto& track : track_infos.value()) {
						tracks.push_front(std::move(track));
					}
				}
			}
			catch (const std::exception& e) {
				XAMP_LOG_DEBUG("Failed to read cue metadata: {} ({})",
					pathToUtf8(cue_path),
					e.what());
			}
			notify_progress();

			sortTracks(tracks);
			if (!tracks.empty()) {
				const auto track_count = countTracks(tracks);
				XAMP_LOG_DEBUG("Metadata scan emit cue path:{} tracks:{} elapsed:{:.3f}s",
					pathToUtf8(cue_path),
					track_count,
					total_elapsed.elapsedSeconds());
				invokeCallback(callbacks.on_tracks, std::move(tracks));
			}
		},
		stop_token);
	XAMP_LOG_DEBUG("Metadata scan read cues count:{} elapsed:{:.3f}s total_elapsed:{:.3f}s",
		files.cue_files.size(),
		stage_elapsed.elapsedSeconds(),
		total_elapsed.elapsedSeconds());

	progress.completed_work = completed_work.load();
	XAMP_LOG_DEBUG("Metadata scan completed path:{} total:{} completed:{} elapsed:{:.3f}s",
		pathToUtf8(root_path),
		total_work,
		progress.completed_work,
		total_elapsed.elapsedSeconds());
	return progress;
}

XAMP_METADATA_NAMESPACE_END
