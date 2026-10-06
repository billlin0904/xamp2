//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <metadata/imetadatascanreader.h>

#include <optional>
#include <atomic>
#include <stdexcept>
#include <utility>

#include <base/executor.h>
#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/threadpoolbuilder.h>
#include <metadata/api.h>
#include <metadata/taglibmetareader.h>

XAMP_METADATA_NAMESPACE_BEGIN

namespace {
XAMP_DECLARE_LOG_NAME(MetadataScanReader);
std::atomic_bool ntfs_scan_enabled{false};

class ParallelMetadataScanReader : public IMetadataScanReader {
public:
	explicit ParallelMetadataScanReader(std::shared_ptr<IThreadPool> thread_pool)
		: thread_pool_(std::move(thread_pool)) {
		if (!thread_pool_) {
			throw std::invalid_argument("Metadata scan thread pool must not be null");
		}
	}

	void read(const MetadataDirectoryFiles& directory_files, const std::stop_token& stop_token,
			  const MetadataReadCallbacks& callbacks) override {
		if (stop_token.stop_requested() || directory_files.empty()) {
			return;
		}
		constexpr auto kIOThreadCount = 8;
		constexpr auto kIOBulkSize = 2;
		auto io_thread_pool = ThreadPoolBuilder::makeThreadPool("IO ThreadPool", kIOThreadCount, kIOBulkSize,
																ThreadPriority::PRIORITY_BACKGROUND);

		std::vector<const MetadataDirectoryFiles::value_type*> directories;
		for (const auto& entry : directory_files) {
			directories.push_back(&entry);
		}
		Executor::parallelFor(
			thread_pool_, directories,
			[&](const auto* directory, const auto& token) {
				const auto& path_info = *directory;
				if (stop_token.stop_requested() || token.stop_requested()) {
					return;
				}

				std::forward_list<TrackInfo> tracks;
				std::vector<const Path*> paths;
				for (const auto& path : path_info.second) {
					paths.push_back(&path);
				}
				auto track_results = Executor::parallelFor(
					io_thread_pool, paths,
					[&](const Path* file_path, const auto& io_stop_token) -> std::optional<TrackInfo> {
						const auto& path = *file_path;
						if (stop_token.stop_requested() || token.stop_requested() ||
							io_stop_token.stop_requested()) {
							return std::nullopt;
						}

						XAMP_ON_SCOPE_EXIT(safeInvoke(callbacks.on_file_completed));

						try {
							return readTrack(path, stop_token);
						} catch (const std::exception& e) {
							XAMP_LOG_DEBUG("Failed to read metadata: {} ({})",
										   String::toString(path.wstring()), e.what());
						}
						return std::nullopt;
					},
					stop_token);

				for (auto& result : track_results) {
					if (result.has_value() && result->has_value()) {
						tracks.push_front(std::move(result->value()));
					}
				}

				safeInvoke(callbacks.on_directory_read, path_info.first, path_info.second.size(),
						   std::move(tracks));
			},
			stop_token);
	}

protected:
	virtual std::optional<TrackInfo> readTrack(const Path& path, const std::stop_token& stop_token) {
		if (stop_token.stop_requested()) {
			return std::nullopt;
		}
		auto reader = makeMetadataReader();
		reader->open(path);
		auto result = reader->extract();
		if (result) {
			return std::move(result.value());
		}
		return std::nullopt;
	}

private:
	std::shared_ptr<IThreadPool> thread_pool_;
};

class NtfsMetadataScanReader final : public ParallelMetadataScanReader {
public:
	using ParallelMetadataScanReader::ParallelMetadataScanReader;

protected:
	std::optional<TrackInfo> readTrack(const Path& path, const std::stop_token& stop_token) override {
		if (stop_token.stop_requested()) {
			return std::nullopt;
		}
#ifdef XAMP_OS_WIN
		try {
			TaglibMetadataReader reader;
			reader.openNtfs(path);
			auto result = reader.extract();
			if (result) {
				XAMP_LOG_DEBUG("NTFS metadata read: {}", String::toString(path.wstring()));
				return std::move(result.value());
			}
			XAMP_LOG_DEBUG("NTFS metadata parse failed, retrying path: {}", String::toString(path.wstring()));
		} catch (const std::exception& e) {
			XAMP_LOG_DEBUG("NTFS metadata fallback: {} ({})", String::toString(path.wstring()), e.what());
		}
#endif
		// The outer per-file scope reports progress once, even after a retry.
		return ParallelMetadataScanReader::readTrack(path, stop_token);
	}
};
} // namespace

void setNtfsMetadataScanEnabled(bool enabled) {
	ntfs_scan_enabled.store(enabled, std::memory_order_relaxed);
}

std::shared_ptr<IMetadataScanReader> makeMetadataScanReader(std::shared_ptr<IThreadPool> thread_pool) {
#ifdef XAMP_OS_WIN
	if (ntfs_scan_enabled.load(std::memory_order_relaxed)) {
		return makeNtfsMetadataScanReader(std::move(thread_pool));
	}
#endif
	return makePathMetadataScanReader(std::move(thread_pool));
}

std::shared_ptr<IMetadataScanReader> makeNtfsMetadataScanReader(std::shared_ptr<IThreadPool> thread_pool) {
	return std::make_shared<NtfsMetadataScanReader>(std::move(thread_pool));
}

std::shared_ptr<IMetadataScanReader> makePathMetadataScanReader(std::shared_ptr<IThreadPool> thread_pool) {
	return std::make_shared<ParallelMetadataScanReader>(std::move(thread_pool));
}

XAMP_METADATA_NAMESPACE_END
