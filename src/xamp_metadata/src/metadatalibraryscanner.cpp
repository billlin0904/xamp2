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
#include <utility>

#include <base/archivefile.h>
#include <base/executor.h>
#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/stopwatch.h>
#include <metadata/api.h>
#include <metadata/cuefilereader.h>

XAMP_METADATA_NAMESPACE_BEGIN

namespace {
	XAMP_DECLARE_LOG_NAME(MetadataLibraryScanner);

	constexpr auto kCueFileExtension = ".cue";
	constexpr auto kZipFileExtension = ".zip";

	struct ScanFiles final {
		HashMap<Path, std::vector<Path>> directory_files;
		std::vector<Path> cue_files;
		std::vector<Path> archive_files;
	};

	std::string PathToUtf8(const Path& path) {
		return String::ToString(path.wstring());
	}

	std::string NormalizeExtension(const Path& path) {
		auto extension = String::ToString(path.extension().wstring());
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
			});
		return extension;
	}

	bool IsSupportedTrackFile(const Path& path) {
		return GetSupportFileExtensions().contains(NormalizeExtension(path));
	}

	void CollectFile(const Path& path, ScanFiles& files, const MetadataScanOptions& options) {
		const auto extension = NormalizeExtension(path);
		if (extension == kCueFileExtension) {
			files.cue_files.push_back(path);
		}
		else if (options.include_archive_files && extension == kZipFileExtension) {
			files.archive_files.push_back(path);
		}
		else if (IsSupportedTrackFile(path)) {
			auto parent_path = path.parent_path();
			auto& directory = files.directory_files[parent_path];
			if (directory.empty()) {
				directory.reserve(1024);
			}
			directory.push_back(path);
		}
	}

	void CollectFiles(const Path& root_path,
		ScanFiles& files,
		const MetadataScanOptions& options,
		const std::stop_token& stop_token) {
		std::error_code ec;
		if (Fs::is_regular_file(root_path, ec)) {
			CollectFile(root_path, files, options);
			return;
		}

		if (!Fs::is_directory(root_path, ec)) {
			return;
		}

		auto collect_entry = [&](const DirectoryEntry& entry) {
			if (entry.is_regular_file(ec)) {
				CollectFile(entry.path(), files, options);
			}
			if (ec) {
				XAMP_LOG_DEBUG("Failed to read path: {} ({})",
					PathToUtf8(entry.path()),
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
						PathToUtf8(root_path),
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
					PathToUtf8(root_path),
					ec.message());
				ec.clear();
			}
		}
	}

	size_t CountArchiveEntries(const std::vector<Path>& archive_files, const std::stop_token& stop_token) {
		size_t count = 0;
		for (const auto& archive_path : archive_files) {
			if (stop_token.stop_requested()) {
				break;
			}

			ArchiveFile archive_file;
			auto result = archive_file.Open(archive_path);
			if (!result) {
				XAMP_LOG_DEBUG("Failed to open archive: {} ({})",
					PathToUtf8(archive_path),
					result.error());
				continue;
			}
			count += result.value().size();
		}
		return count;
	}

	size_t CountScanFiles(const ScanFiles& files, const std::stop_token& stop_token) {
		size_t total_work = files.cue_files.size();
		for (const auto& [directory, paths] : files.directory_files) {
			total_work += paths.size();
		}
		total_work += CountArchiveEntries(files.archive_files, stop_token);
		return total_work;
	}

	void SortTracks(std::forward_list<TrackInfo>& tracks) {
		tracks.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
			});
	}

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

	template <typename Callback, typename... Args>
	void InvokeCallback(const Callback& callback, Args&&... args) {
		if (callback) {
			std::invoke(callback, std::forward<Args>(args)...);
		}
	}
}

MetadataLibraryScanner::MetadataLibraryScanner(std::shared_ptr<IThreadPoolExecutor> thread_pool)
	: thread_pool_(std::move(thread_pool)) {
	XAMP_ENSURES(thread_pool_ != nullptr);
}

MetadataScanProgress MetadataLibraryScanner::Scan(const Path& root_path,
	const std::stop_token& stop_token,
	const MetadataScanCallbacks& callbacks,
	const MetadataScanOptions& options) {
	Stopwatch total_elapsed;
	Stopwatch stage_elapsed;

	ScanFiles files;
	CollectFiles(root_path, files, options, stop_token);
	const auto collect_seconds = stage_elapsed.ElapsedSeconds();

	stage_elapsed.Reset();
	const auto total_work = CountScanFiles(files, stop_token);
	const auto count_seconds = stage_elapsed.ElapsedSeconds();
	InvokeCallback(callbacks.on_found_file_count, total_work);

	XAMP_LOG_DEBUG("Metadata scan prepare path:{} total:{} directories:{} cues:{} archives:{} collect:{:.3f}s count:{:.3f}s",
		PathToUtf8(root_path),
		total_work,
		files.directory_files.size(),
		files.cue_files.size(),
		files.archive_files.size(),
		collect_seconds,
		count_seconds);

	MetadataScanProgress progress;
	progress.total_work = total_work;
	if (total_work == 0 || stop_token.stop_requested()) {
		XAMP_LOG_DEBUG("Metadata scan completed path:{} total:{} completed:{} elapsed:{:.3f}s",
			PathToUtf8(root_path),
			total_work,
			progress.completed_work,
			total_elapsed.ElapsedSeconds());
		return progress;
	}

	std::atomic<size_t> completed_work = 0;
	auto notify_progress = [&]() {
		const auto completed = ++completed_work;
		MetadataScanProgress progress;
		progress.total_work = total_work;
		progress.completed_work = completed;
		InvokeCallback(callbacks.on_progress, progress);
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
		const auto track_count = CountTrackBatches(batch);
		XAMP_LOG_DEBUG("Metadata scan emit batch path:{} directories:{} tracks:{} elapsed:{:.3f}s",
			PathToUtf8(path),
			batch_size,
			track_count,
			total_elapsed.ElapsedSeconds());

		InvokeCallback(callbacks.on_read_path, path, path_size);
		InvokeCallback(callbacks.on_batch_tracks, std::move(batch));
		};

	stage_elapsed.Reset();
	Executor::ParallelForEach(thread_pool_,
		files.directory_files,
		[&](auto& path_info, const auto& token) {
			if (stop_token.stop_requested() || token.stop_requested()) {
				return;
			}

			std::forward_list<TrackInfo> tracks;
			size_t local_track_count = 0;
			auto reader = MakeMetadataReader();

			for (const auto& path : path_info.second) {
				if (stop_token.stop_requested() || token.stop_requested()) {
					return;
				}

				try {
					reader->Open(path);
					auto track_info = reader->Extract();
					if (track_info) {
						tracks.push_front(std::move(track_info.value()));
						++local_track_count;
					}
				}
				catch (const std::exception& e) {
					XAMP_LOG_DEBUG("Failed to read metadata: {} ({})",
						PathToUtf8(path),
						e.what());
				}
				notify_progress();
			}

			SortTracks(tracks);
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
		stage_elapsed.ElapsedSeconds(),
		total_elapsed.ElapsedSeconds());

	flush_batch(root_path, 0, true);

	stage_elapsed.Reset();
	Executor::ParallelForEach(thread_pool_,
		files.archive_files,
		[&](auto& archive_path, const auto& token) {
			if (stop_token.stop_requested() || token.stop_requested()) {
				return;
			}

			ArchiveFile archive_file;
			auto result = archive_file.Open(archive_path);
			if (!result) {
				XAMP_LOG_DEBUG("Failed to open archive: {} ({})",
					PathToUtf8(archive_path),
					result.error());
				return;
			}

			auto reader = MakeMetadataReader();
			std::forward_list<TrackInfo> tracks;
			for (const auto& entry_name : result.value()) {
				if (stop_token.stop_requested() || token.stop_requested()) {
					return;
				}

				auto entry = archive_file.GetEntryByName(entry_name);
				try {
					if (entry) {						
						reader->Open(std::move(entry.value()));
						auto track_info = reader->Extract();
						if (track_info) {
							tracks.push_front(std::move(track_info.value()));
						}
					}
				}
				catch (const std::exception& e) {
					XAMP_LOG_DEBUG("Failed to read archive metadata: {} ({})",
						PathToUtf8(archive_path),
						e.what());
				}
				notify_progress();
			}

			SortTracks(tracks);
			if (!tracks.empty()) {
				const auto track_count = CountTracks(tracks);
				XAMP_LOG_DEBUG("Metadata scan emit archive path:{} tracks:{} elapsed:{:.3f}s",
					PathToUtf8(archive_path),
					track_count,
					total_elapsed.ElapsedSeconds());
				InvokeCallback(callbacks.on_tracks, std::move(tracks));
			}
		},
		stop_token);
	XAMP_LOG_DEBUG("Metadata scan read archives count:{} elapsed:{:.3f}s total_elapsed:{:.3f}s",
		files.archive_files.size(),
		stage_elapsed.ElapsedSeconds(),
		total_elapsed.ElapsedSeconds());

	FastMutex cue_mutex;
	stage_elapsed.Reset();
	Executor::ParallelForEach(thread_pool_,
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
				auto track_infos = loader.Load(cue_path);
				if (track_infos) {
					for (auto& track : track_infos.value()) {
						tracks.push_front(std::move(track));
					}
				}
			}
			catch (const std::exception& e) {
				XAMP_LOG_DEBUG("Failed to read cue metadata: {} ({})",
					PathToUtf8(cue_path),
					e.what());
			}
			notify_progress();

			SortTracks(tracks);
			if (!tracks.empty()) {
				const auto track_count = CountTracks(tracks);
				XAMP_LOG_DEBUG("Metadata scan emit cue path:{} tracks:{} elapsed:{:.3f}s",
					PathToUtf8(cue_path),
					track_count,
					total_elapsed.ElapsedSeconds());
				InvokeCallback(callbacks.on_tracks, std::move(tracks));
			}
		},
		stop_token);
	XAMP_LOG_DEBUG("Metadata scan read cues count:{} elapsed:{:.3f}s total_elapsed:{:.3f}s",
		files.cue_files.size(),
		stage_elapsed.ElapsedSeconds(),
		total_elapsed.ElapsedSeconds());

	progress.completed_work = completed_work.load();
	XAMP_LOG_DEBUG("Metadata scan completed path:{} total:{} completed:{} elapsed:{:.3f}s",
		PathToUtf8(root_path),
		total_work,
		progress.completed_work,
		total_elapsed.ElapsedSeconds());
	return progress;
}

XAMP_METADATA_NAMESPACE_END
