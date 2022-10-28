//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <base/align_ptr.h>
#include <base/fs.h>
#include <metadata/metadata.h>

namespace xamp::metadata {

XAMP_METADATA_API AlignPtr<IMetadataReader> MakeMetadataReader();

XAMP_METADATA_API AlignPtr<IMetadataWriter> MakeMetadataWriter();

XAMP_METADATA_API void ScanFolder(Path const& path, IMetadataExtractAdapter* adapter, IMetadataReader* reader, bool is_recursive = true);

XAMP_METADATA_API void ScanFolder(Path const& path,
	const std::function<bool(const Path&)>& is_accept,
	const std::function<void(const Path&)>& walk,
	const std::function<void(const Path&, bool)>& end_walk,
	bool is_recursive = true);

}

