//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <utility>

#include <base/metadata.h>
#include <base/audioformat.h>

using xamp::base::Metadata;
using xamp::base::AudioFormat;
using xamp::base::Path;

namespace read_utiltis {

double readAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process = nullptr,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());
	
std::tuple<double, double> readFileLUFS(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());

void encodeFlacFile(Path const& file_path,
    Path const& output_file_path,
    std::wstring const& command,
    std::function<bool(uint32_t)> const& progress,
    Metadata const& metadata);

}
