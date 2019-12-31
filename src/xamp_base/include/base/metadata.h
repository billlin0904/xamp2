//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <string>

#include <base/base.h>

namespace xamp::base {

struct XAMP_BASE_API Metadata final {
    Metadata();
    int32_t track;
    int32_t bitrate;
	int32_t samplerate;
	double offset;
    double duration;
    std::wstring file_path;
    std::wstring file_name;
    std::wstring file_name_no_ext;
    std::wstring file_ext;
    std::wstring title;
    std::wstring artist;
    std::wstring album;
    std::wstring parent_path;
	std::string cover_id;
};

}
