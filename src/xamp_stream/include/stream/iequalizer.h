//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

class XAMP_NO_VTABLE XAMP_STREAM_API IEqualizer : public IAudioProcessor {
public:
    constexpr static auto Id = std::string_view("FCC73B23-6806-44CD-882D-EA21A3482F51");

    XAMP_BASE_CLASS(IEqualizer)

    virtual void SetEQ(uint32_t band, float gain, float Q) = 0;

    virtual void SetEQ(EQSettings const &settings) = 0;

    virtual void SetPreamp(float preamp) = 0;
protected:
    IEqualizer() = default;
};

}
