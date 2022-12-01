//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/compressorparameters.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

class XAMP_STREAM_API BassCompressor final : public IAudioProcessor {
public:
    constexpr static auto Id = std::string_view("263079D0-FDD4-46DF-9BB3-71821AF95EDB");    
	
    BassCompressor();

    XAMP_PIMPL(BassCompressor)

    void Start(const DspConfig& config) override;

    void Init(const DspConfig& config) override;

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassCompressorImpl;
    AlignPtr<BassCompressorImpl> impl_;
};

}

