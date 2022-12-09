//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <any>
#include <map>

#include <base/base.h>
#include <base/uuid.h>
#include <base/buffer.h>
#include <base/UuidOf.h>

#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API DspConfig {
public:
    constexpr static auto kInputFormat = std::string_view("InputFormat");
    constexpr static auto kDsdMode = std::string_view("DsdMode");
    constexpr static auto kOutputFormat = std::string_view("OutputFormat");
    constexpr static auto kSampleSize = std::string_view("SampleSize");
    constexpr static auto kEQSettings = std::string_view("EQSettings");
    constexpr static auto kCompressorParameters = std::string_view("CompressorParameters");
    constexpr static auto kVolume = std::string_view("Volume");

    template <typename T>
    void AddOrReplace(const std::string_view &name, T&& value) {
        configs_.insert_or_assign(name, std::forward<T>(value));
    }

    template <typename T>
    T Get(const std::string_view& name) const {
        return std::any_cast<T>(configs_.at(name));
    }
private:
    std::map<std::string_view, std::any> configs_;
};

class XAMP_NO_VTABLE XAMP_STREAM_API IAudioProcessor {
public:
	XAMP_BASE_CLASS(IAudioProcessor)

    virtual void Start(const DspConfig& config) = 0;

    virtual void Init(const DspConfig& config) = 0;

    virtual bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) = 0;

    virtual uint32_t Process(float const* samples, float* out, uint32_t num_samples) = 0;

    [[nodiscard]] virtual Uuid GetTypeId() const = 0;

    [[nodiscard]] virtual std::string_view GetDescription() const noexcept = 0;

    virtual void Flush() = 0;
protected:
	IAudioProcessor() = default;
};

}

