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

    virtual void setDSDMode(DsdModes mode) = 0;

    [[nodiscard]] virtual DsdModes getDsdMode() const = 0;

    [[nodiscard]] virtual uint32_t getDsdSampleRate() const = 0;

    [[nodiscard]] virtual DsdFormat getDsdFormat() const = 0;

    virtual void setDsdToPcmSampleRate(uint32_t sample_rate) = 0;

    [[nodiscard]] virtual uint32_t getDsdSpeed() const = 0;

    [[nodiscard]] virtual uint32_t getBitRate() const = 0;

    [[nodiscard]] virtual bool isDsdFile() const = 0;

    [[nodiscard]] virtual bool supportDOP() const = 0;

    [[nodiscard]] virtual bool supportDOP_AA() const = 0;

    [[nodiscard]] virtual bool supportNativeSD() const = 0;
protected:
    IDsdStream() = default;
};

XAMP_STREAM_NAMESPACE_END

