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
#include <base/stl.h>
#include <base/threadpool.h>
#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

using MetadataDirectoryFiles = HashMap<Path, std::vector<Path>>;

struct MetadataReadCallbacks final {
	// Called once per attempted file, including failed metadata extraction.
	std::function<void()> on_file_completed;
	// Called once per processed directory, including directories with no tracks.
	std::function<void(const Path&, size_t, std::forward_list<TrackInfo>)> on_directory_read;
};

// Implementations must finish all work before returning and honor cancellation.
// Callbacks may run concurrently and must be thread-safe. Input paths remain
// valid only for the duration of read(); implementations must not retain them.
class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataScanReader {
  public:
	virtual ~IMetadataScanReader() = default;

	virtual void read(const MetadataDirectoryFiles& directory_files, const std::stop_token& stop_token,
					  const MetadataReadCallbacks& callbacks) = 0;
};

// Configure the backend for subsequently created default readers at startup.
XAMP_METADATA_API void setNtfsMetadataScanEnabled(bool enabled);

// NTFS reads are opt-in on Windows, with per-file fallback. Otherwise use paths.
XAMP_METADATA_API std::shared_ptr<IMetadataScanReader> makeMetadataScanReader(
	std::shared_ptr<IThreadPool> thread_pool);

// Explicit choices for callers that want to select the backend.
XAMP_METADATA_API std::shared_ptr<IMetadataScanReader> makeNtfsMetadataScanReader(
	std::shared_ptr<IThreadPool> thread_pool);
XAMP_METADATA_API std::shared_ptr<IMetadataScanReader> makePathMetadataScanReader(
	std::shared_ptr<IThreadPool> thread_pool);

XAMP_METADATA_NAMESPACE_END
