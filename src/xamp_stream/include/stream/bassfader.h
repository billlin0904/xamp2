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
    BassFader();

    XAMP_PIMPL(BassFader)

    void Start(const DspConfig& config) override;

    void Init(const DspConfig& config) override;

    void SetTime(float current, float target, float fdade_time);

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassFaderImpl;
    AlignPtr<BassFaderImpl> impl_;
};
XAMP_MAKE_CLASS_UUID(BassFader, "6981FB2E-E133-4B33-9378-F23628EEE4FE")

}

