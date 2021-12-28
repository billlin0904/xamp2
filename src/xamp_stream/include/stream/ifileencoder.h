//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE IFileEncoder {
public:
	XAMP_BASE_CLASS(IFileEncoder)

	virtual void Start(std::wstring const& input_file_path, std::wstring const& output_file_path, std::wstring const& command) = 0;

	virtual void Encode(std::function<bool(uint32_t)> const& progress) = 0;

protected:
	IFileEncoder() = default;
};

}
