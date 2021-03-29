//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <base/align_ptr.h>
#include <stream/stream.h>

namespace xamp::stream {

XAMP_STREAM_API bool TestDsdFileFormatStd(std::wstring const& file_path);

XAMP_STREAM_API bool TestDsdFileFormat(std::wstring const& file_path);

XAMP_STREAM_API HashSet<std::string> const & GetSupportFileExtensions();

XAMP_STREAM_API DsdStream * AsDsdStream(AlignPtr<FileStream> const & stream) noexcept;
	
XAMP_STREAM_API AlignPtr<FileStream> MakeStream(std::wstring const& file_ext, AlignPtr<FileStream> old_stream = nullptr);


}
