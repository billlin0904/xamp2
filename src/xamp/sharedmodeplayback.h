//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>

#include <cstdint>

struct SharedModePlaybackConfig final {
    uint32_t target_sample_rate{ 0 };
    ByteFormat byte_format{ ByteFormat::SINT32 };
    bool needs_resample{ false };
};

struct PlaybackPlan final {
    DsdModes output_mode{ DsdModes::DSD_MODE_PCM };
    ByteFormat byte_format{ ByteFormat::SINT32 };
    uint32_t target_sample_rate{ 0 };
    bool use_mqa_decode{ false };
    bool needs_resample{ false };
    bool is_shared_device{ false };
};

[[nodiscard]] uint32_t resolveSharedModeTargetSampleRate(
    const DeviceInfo& device_info,
    uint32_t input_sample_rate,
    DsdModes output_mode);

[[nodiscard]] SharedModePlaybackConfig resolveSharedModePlaybackConfig(
    const DeviceInfo& device_info,
    uint32_t input_sample_rate,
    DsdModes output_mode,
    ByteFormat default_byte_format);

[[nodiscard]] PlaybackPlan resolvePlaybackPlan(
    const DeviceInfo& device_info,
    uint32_t input_sample_rate,
    bool is_dsd_file,
    bool is_shared_device,
    bool is_asio_device);

[[nodiscard]] ByteFormat resolvePreparedPlaybackByteFormat(
    const PlaybackPlan& plan,
    bool is_mqa_stream);
