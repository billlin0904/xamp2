//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

class XAMP_STREAM_API BassFader final : public IAudioProcessor {
public:
    constexpr static auto Id = std::string_view("6981FB2E-E133-4B33-9378-F23628EEE4FE");    
	
    BassFader();

    XAMP_PIMPL(BassFader)

    void Start(uint32_t output_sample_rate) override;

    void Init(float current = 1.0f, float target = 0.0f, float time = 3);

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassFaderImpl;
    AlignPtr<BassFaderImpl> impl_;
};

}

