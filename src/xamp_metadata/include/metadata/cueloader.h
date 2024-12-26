//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/trackinfo.h>
#include <base/memory.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

class XAMP_METADATA_API CueLoader {
public:
	CueLoader();

	XAMP_PIMPL(CueLoader)

	std::vector<TrackInfo> Load(const Path& file_path);
private:
	class CueLoaderImpl;
	ScopedPtr<CueLoaderImpl> impl_;
};

XAMP_METADATA_NAMESPACE_END

