//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

    virtual void SetDSDMode(DsdModes mode) noexcept = 0;

    XAMP_NO_DISCARD virtual DsdModes GetDsdMode() const noexcept = 0;

    XAMP_NO_DISCARD virtual uint32_t GetDsdSampleRate() const = 0;

    XAMP_NO_DISCARD virtual DsdFormat GetDsdFormat() const noexcept = 0;

    virtual void SetDsdToPcmSampleRate(uint32_t sample_rate) = 0;

    XAMP_NO_DISCARD virtual uint32_t GetDsdSpeed() const = 0;

    XAMP_NO_DISCARD virtual bool IsDsdFile() const noexcept = 0;

    XAMP_NO_DISCARD virtual bool SupportDOP() const noexcept = 0;

    XAMP_NO_DISCARD virtual bool SupportDOP_AA() const noexcept = 0;

    XAMP_NO_DISCARD virtual bool SupportNativeSD() const noexcept = 0;
protected:
    IDsdStream() = default;
};

XAMP_STREAM_NAMESPACE_END

