//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

class XAMP_STREAM_API BassVolume final : public IAudioProcessor {
public:
    constexpr static auto Id = std::string_view("83d25234-5484-45a3-bda8-baf35541f9d2");    
	
    BassVolume();

    XAMP_PIMPL(BassVolume)

    void Start(uint32_t sample_rate) override;

    void Init(float volume);

    bool Process(float const * samples, uint32_t num_samples, Buffer<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

private:
    class BassVolumeImpl;
    AlignPtr<BassVolumeImpl> impl_;
};

}