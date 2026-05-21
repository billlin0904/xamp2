//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/audiobuffer.h>
#include <base/memory.h>

XAMP_STREAM_NAMESPACE_BEGIN

inline constexpr int32_t kR8brainBufferSize = 64 * 1024;

class XAMP_STREAM_API R8brainSampleRateConverter final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(R8brainSampleRateConverter, "786D706E-20F0-4F30-9B98-8B489DC5C739")

public:
    XAMP_PIMPL(R8brainSampleRateConverter)

    XAMP_DECLARE_UUID_CLASS(R8brainSampleRateConverter)

	R8brainSampleRateConverter();

    void Initialize(const Property& config) override;

    [[nodiscard]] bool Process(float const* samples, size_t num_samples, BufferRef<float>& output) override;    

private:
    class R8brainSampleRateConverterImpl;
    ScopedPtr<R8brainSampleRateConverterImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
