//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/metadata.h>
#include <base/memory.h>
#include <base/fs.h>

XAMP_METADATA_NAMESPACE_BEGIN

struct CueTrack {
	Path file_path;
	std::chrono::milliseconds timestamp;
};	

class XAMP_METADATA_API CueFileReader final {
public:
	CueFileReader();

	XAMP_PIMPL(CueFileReader)

	void Open(const Path& path);

	std::vector<CueTrack> GetTracks() const;
private:
	class CueFileReaderImpl;
	AlignPtr<CueFileReaderImpl> impl_;
};

XAMP_METADATA_NAMESPACE_END
