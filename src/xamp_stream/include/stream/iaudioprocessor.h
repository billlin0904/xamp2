//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/anymap.h>

#include <base/base.h>
#include <base/uuid.h>
#include <base/buffer.h>

XAMP_STREAM_NAMESPACE_BEGIN

/*
* DspConfig is a struct for audio processor configuration.
* 
*/
namespace DspConfig {
    constexpr static auto kInputFormat      = std::string_view("InputFormat");
    constexpr static auto kDsdMode          = std::string_view("DsdMode");
    constexpr static auto kOutputFormat     = std::string_view("OutputFormat");
    constexpr static auto kSampleSize       = std::string_view("SampleSize");
    constexpr static auto kEQSettings       = std::string_view("EQSettings");
    constexpr static auto kCompressorConfig = std::string_view("CompressorConfig");
    constexpr static auto kVolume           = std::string_view("Volume");
};

/*
* IAudioProcessor is an interface for audio processor.
* 
*/
class XAMP_NO_VTABLE XAMP_STREAM_API IAudioProcessor {
public:
	XAMP_BASE_CLASS(IAudioProcessor)

    /*
    * Initialize the audio processor.
    * 
    * @param config: the configuration for the audio processor.
    */
    virtual void Initialize(const AnyMap& config) = 0;

    /*
    * Process the audio samples.
    * 
    * @param samples: the input audio samples.
    * @param num_samples: the number of input audio samples.
    * @param output: the output audio samples.
    * 
    * @return: true if success, otherwise false.
    */
    virtual bool Process(float const* samples, size_t num_samples, BufferRef<float>& output) = 0;

    /*
    * Get the type id of the audio processor.
    * 
    * @return the type id of the audio processor.
    */
    [[nodiscard]] virtual Uuid GetTypeId() const = 0;

    /*
    * Get the description of the audio processor.
    * 
    * @return the description of the audio processor.
    */
    [[nodiscard]] virtual std::string_view GetDescription() const noexcept = 0;

protected:
	IAudioProcessor() = default;
};

XAMP_STREAM_NAMESPACE_END

