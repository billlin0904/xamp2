//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API BassCompressor final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassCompressor, "263079D0-FDD4-46DF-9BB3-71821AF95EDB")

public:
    XAMP_DECLARE_UUID_CLASS(BassCompressor)

    BassCompressor();

    XAMP_PIMPL(BassCompressor)

    void Initialize(const Property& config) override;

    bool Process(float const * samples, size_t num_samples, BufferRef<float>& out) override;

private:
    class BassCompressorImpl;
    ScopedPtr<BassCompressorImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
