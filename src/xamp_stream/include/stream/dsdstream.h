//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dsdsampleformat.h>

#include <stream/stream.h>

namespace xamp::stream {

using namespace base;

class XAMP_STREAM_API XAMP_NO_VTABLE DSDStream {
public:
    XAMP_BASE_CLASS(DSDStream)

    virtual bool SupportDOP() const noexcept = 0;

    virtual bool SupportDOP_AA() const noexcept = 0;

    virtual bool SupportRAW() const noexcept = 0;

    virtual void SetDSDMode(DSDModes mode) noexcept = 0;

    virtual DSDModes GetDSDMode() const noexcept = 0;

    virtual int32_t GetDSDSampleRate() const = 0;

	virtual DSDSampleFormat GetDSDSampleFormat() const noexcept = 0;

	virtual void SetPCMSampleRate(int32_t samplerate) = 0;

    virtual int32_t GetDSDSpeed() const = 0;

	virtual bool IsDSDFile() const noexcept = 0;
protected:
    DSDStream() = default;
};

}

