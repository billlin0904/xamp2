//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/base.h>
#include <base/dsdsampleformat.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API XAMP_NO_VTABLE IDsdStream {
public:
    XAMP_BASE_CLASS(IDsdStream)

    virtual void SetDSDMode(DsdModes mode) = 0;

    [[nodiscard]] virtual DsdModes GetDsdMode() const = 0;

    [[nodiscard]] virtual uint32_t GetDsdSampleRate() const = 0;

    [[nodiscard]] virtual DsdFormat GetDsdFormat() const = 0;

    virtual void SetDsdToPcmSampleRate(uint32_t sample_rate) = 0;

    [[nodiscard]] virtual uint32_t GetDsdSpeed() const = 0;

    [[nodiscard]] virtual uint32_t GetBitRate() const = 0;

    [[nodiscard]] virtual bool IsDsdFile() const = 0;

    [[nodiscard]] virtual bool SupportDOP() const = 0;

    [[nodiscard]] virtual bool SupportDOP_AA() const = 0;

    [[nodiscard]] virtual bool SupportNativeSD() const = 0;
protected:
    IDsdStream() = default;
};

XAMP_STREAM_NAMESPACE_END

