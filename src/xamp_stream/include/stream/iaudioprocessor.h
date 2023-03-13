//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/anymap.h>

#include <base/base.h>
#include <base/uuid.h>
#include <base/buffer.h>

namespace xamp::stream {

struct XAMP_STREAM_API DspConfig {
    constexpr static auto kInputFormat = std::string_view("InputFormat");
    constexpr static auto kDsdMode = std::string_view("DsdMode");
    constexpr static auto kOutputFormat = std::string_view("OutputFormat");
    constexpr static auto kSampleSize = std::string_view("SampleSize");
    constexpr static auto kEQSettings = std::string_view("EQSettings");
    constexpr static auto kCompressorParameters = std::string_view("CompressorParameters");
    constexpr static auto kVolume = std::string_view("Volume");
    constexpr static auto kParametricEqBand = std::string_view("ParametricEqBand");
};

class XAMP_NO_VTABLE XAMP_STREAM_API IAudioProcessor {
public:
	XAMP_BASE_CLASS(IAudioProcessor)

    virtual void Start(const AnyMap& config) = 0;

    virtual void Init(const AnyMap& config) = 0;

    virtual bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) = 0;

    virtual uint32_t Process(float const* samples, float* out, uint32_t num_samples) = 0;

    [[nodiscard]] virtual Uuid GetTypeId() const = 0;

    [[nodiscard]] virtual std::string_view GetDescription() const noexcept = 0;

    virtual void Flush() = 0;
protected:
	IAudioProcessor() = default;
};

}

