//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <set>
#include <base/stl.h>
#include <base/align_ptr.h>
#include <stream/stream.h>

namespace xamp::stream {

XAMP_STREAM_API bool TestDsdFileFormatStd(std::wstring const& file_path);

XAMP_STREAM_API HashSet<std::string> const& GetSupportFileExtensions();

XAMP_STREAM_API IDsdStream* AsDsdStream(AlignPtr<FileStream> const & stream) noexcept;
	
XAMP_STREAM_API AlignPtr<FileStream> MakeStream();

XAMP_STREAM_API AlignPtr<IFileEncoder> MakeEncoder();

#ifdef XAMP_OS_WIN
XAMP_STREAM_API AlignPtr<ICDDevice> MakeCDDevice(int32_t driver_letter);
#endif

}