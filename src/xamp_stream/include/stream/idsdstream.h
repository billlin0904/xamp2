//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dsdsampleformat.h>

#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE IDsdStream {
public:
    XAMP_BASE_CLASS(IDsdStream)

    virtual void SetDSDMode(DsdModes mode) noexcept = 0;

    [[nodiscard]] virtual DsdModes GetDsdMode() const noexcept = 0;

    [[nodiscard]] virtual uint32_t GetDsdSampleRate() const = 0;

    [[nodiscard]] virtual DsdFormat GetDsdFormat() const noexcept = 0;

    virtual void SetDsdToPcmSampleRate(uint32_t sample_rate) = 0;

    [[nodiscard]] virtual uint32_t GetDsdSpeed() const noexcept = 0;

    [[nodiscard]] virtual bool IsDsdFile() const noexcept = 0;
protected:
    IDsdStream() = default;
};

}
