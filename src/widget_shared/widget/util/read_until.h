//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <utility>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

#include <base/memory.h>
#include <base/trackinfo.h>
#include <base/audioformat.h>
#include <stream/ifileencoder.h>

namespace read_util {

inline constexpr uint32_t kReadSampleSize = 8192 * 4;

double readAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process = nullptr,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());
	
std::tuple<double, double> readFileLufs(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());

std::tuple<double, std::vector<uint8_t>> readFingerprint(Path const& file_path);

XAMP_WIDGET_SHARED_EXPORT void encodeFile(AnyMap const &config,
    AlignPtr<IFileEncoder>& encoder,
    std::function<bool(uint32_t)> const& progress,
    TrackInfo const& track_info);

}
