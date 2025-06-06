//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <expected>

#include <base/fs.h>
#include <base/trackinfo.h>
#include <base/memory.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

enum class ParseCueError {
	PARSE_ERROR,
	PARSE_ERROR_UNKNOWN_ENCODING,
	PARSE_ERROR_NOT_FOUND_FILE_NAME,
	PARSE_ERROR_READ_TRACK_INFO,
};

class XAMP_METADATA_API CueLoader {
public:	
	CueLoader();

	XAMP_PIMPL(CueLoader)

	std::expected<std::vector<TrackInfo>, ParseCueError> Load(const Path& file_path);
private:
	class CueLoaderImpl;
	ScopedPtr<CueLoaderImpl> impl_;
};

XAMP_METADATA_NAMESPACE_END

