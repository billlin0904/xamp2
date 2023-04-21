//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <utility>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

#include <base/align_ptr.h>
#include <base/trackinfo.h>
#include <base/audioformat.h>
#include <stream/ifileencoder.h>

namespace read_utiltis {

XAMP_WIDGET_SHARED_EXPORT double ReadAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process = nullptr,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());
	
XAMP_WIDGET_SHARED_EXPORT  std::tuple<double, double> ReadFileLufs(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());

XAMP_WIDGET_SHARED_EXPORT void EncodeFile(AnyMap const &config,
    AlignPtr<IFileEncoder>& encoder,
    std::function<bool(uint32_t)> const& progress,
    TrackInfo const& track_info);

XAMP_WIDGET_SHARED_EXPORT void EncodeFile(Path const& file_path,
    Path const& output_file_path,
    AlignPtr<IFileEncoder>& encoder,
    std::wstring const& command,
    std::function<bool(uint32_t)> const& progress,
    TrackInfo const& track_info);

}
