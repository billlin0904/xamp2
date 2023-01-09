//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <utility>

#include <base/align_ptr.h>
#include <base/trackinfo.h>
#include <base/audioformat.h>
#include <stream/ifileencoder.h>

using xamp::base::AlignPtr;
using xamp::base::TrackInfo;
using xamp::base::AudioFormat;
using xamp::base::Path;
using xamp::stream::IFileEncoder;
using xamp::stream::AnyMap;

namespace read_utiltis {

double readAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process = nullptr,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());
	
std::tuple<double, double> readFileLUFS(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());

void encodeFile(AnyMap const &config,
    AlignPtr<IFileEncoder>& encoder,
    std::function<bool(uint32_t)> const& progress,
    TrackInfo const& track_info);

void encodeFile(Path const& file_path,
    Path const& output_file_path,
    AlignPtr<IFileEncoder>& encoder,
    std::wstring const& command,
    std::function<bool(uint32_t)> const& progress,
    TrackInfo const& track_info);

}
