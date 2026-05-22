//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <forward_list>
#include <functional>
#include <memory>
#include <stop_token>
#include <vector>

#include <base/fs.h>
#include <base/ithreadpoolexecutor.h>
#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

struct MetadataScanProgress final {
	size_t total_work{ 0 };
	size_t completed_work{ 0 };
};

struct MetadataScanOptions final {
	size_t batch_size{ 250 };
	bool include_archive_files{ true };
	bool recursive{ true };
};

struct MetadataScanCallbacks final {
	std::function<void(size_t)> on_found_file_count;
	std::function<void(const Path&, size_t)> on_read_path;
	std::function<void(MetadataScanProgress)> on_progress;
	std::function<void(std::vector<std::forward_list<TrackInfo>>)> on_batch_tracks;
	std::function<void(std::forward_list<TrackInfo>)> on_tracks;
};

class XAMP_METADATA_API MetadataLibraryScanner final {
public:
	explicit MetadataLibraryScanner(std::shared_ptr<IThreadPoolExecutor> thread_pool);

	MetadataScanProgress Scan(const Path& root_path,
		const std::stop_token& stop_token,
		const MetadataScanCallbacks& callbacks,
		const MetadataScanOptions& options = {});

private:
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
};

XAMP_METADATA_NAMESPACE_END
