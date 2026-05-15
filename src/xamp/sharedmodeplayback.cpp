//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <sharedmodeplayback.h>

#include <algorithm>

namespace {
    const uint32_t kFallbackSharedModeSampleRate = AudioFormat::k16BitPCM48Khz.GetSampleRate();
    const uint32_t kBluetoothMaxSampleRate = AudioFormat::k16BitPCM48Khz.GetSampleRate();
}

uint32_t resolveSharedModeTargetSampleRate(
    const DeviceInfo& device_info,
    uint32_t input_sample_rate,
    DsdModes output_mode) {
    if (!IsPcmAudio(output_mode)) {
        return 0;
    }

    auto target_sample_rate = kFallbackSharedModeSampleRate;

    if (device_info.default_format.has_value()) {
        target_sample_rate = device_info.default_format->GetSampleRate();
    }
    else if (input_sample_rate != 0) {
        target_sample_rate = input_sample_rate;
    }

    if (device_info.connect_type == DeviceConnectType::BLUE_TOOTH) {
        target_sample_rate = (std::min)(target_sample_rate, kBluetoothMaxSampleRate);
        XAMP_LOG_DEBUG("Bluetooth shared mode target sample rate resolved to {} Hz (input:{} Hz).",
            target_sample_rate,
            input_sample_rate);
    }
    else {
        XAMP_LOG_DEBUG("Shared mode target sample rate resolved to {} Hz (input:{} Hz).",
            target_sample_rate,
            input_sample_rate);
    }

    return target_sample_rate;
}

SharedModePlaybackConfig resolveSharedModePlaybackConfig(
    const DeviceInfo& device_info,
    uint32_t input_sample_rate,
    DsdModes output_mode,
    ByteFormat default_byte_format) {
    SharedModePlaybackConfig config;
    config.byte_format = default_byte_format;
    config.target_sample_rate = resolveSharedModeTargetSampleRate(device_info,
        input_sample_rate,
        output_mode);

    if (device_info.connect_type == DeviceConnectType::BLUE_TOOTH) {
        config.byte_format = ByteFormat::SINT16;
    }

    config.needs_resample = IsPcmAudio(output_mode)
        && config.target_sample_rate != 0
        && config.target_sample_rate != input_sample_rate;

    return config;
}

PlaybackPlan resolvePlaybackPlan(
    const DeviceInfo& device_info,
    uint32_t input_sample_rate,
    bool is_dsd_file,
    bool is_shared_device,
    bool is_asio_device) {
    PlaybackPlan plan;
    plan.is_shared_device = is_shared_device;

    if (is_dsd_file) {
        if (plan.is_shared_device || device_info.connect_type == DeviceConnectType::BLUE_TOOTH) {
            plan.output_mode = DsdModes::DSD_MODE_DSD2PCM;
        }
        else {
            plan.output_mode = is_asio_device
                ? DsdModes::DSD_MODE_NATIVE
                : DsdModes::DSD_MODE_DOP;
        }
    }

    if (plan.is_shared_device) {
        const auto shared_mode_config = resolveSharedModePlaybackConfig(device_info,
            input_sample_rate,
            plan.output_mode,
            plan.byte_format);
        plan.target_sample_rate = shared_mode_config.target_sample_rate;
        plan.byte_format = shared_mode_config.byte_format;
        plan.needs_resample = shared_mode_config.needs_resample;
        return plan;
    }

    plan.byte_format = ByteFormat::SINT24;
    plan.use_mqa_decode = true;
    return plan;
}

ByteFormat resolvePreparedPlaybackByteFormat(
    const PlaybackPlan& plan,
    bool is_mqa_stream) {
    if (!is_mqa_stream) {
        return ByteFormat::SINT32;
    }
    return plan.byte_format;
}
