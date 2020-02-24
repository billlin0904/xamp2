//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dsdsampleformat.h>

#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE DsdStream {
public:
    XAMP_BASE_CLASS(DsdStream)

    virtual DsdModes GetSupportDsdMode() const noexcept = 0;

    virtual void SetDSDMode(DsdModes mode) noexcept = 0;

    virtual DsdModes GetDsdMode() const noexcept = 0;

    virtual int32_t GetDsdSampleRate() const = 0;

	virtual DsdSampleFormat GetDsdSampleFormat() const noexcept = 0;

	virtual void SetDsdToPcmSampleRate(int32_t samplerate) = 0;

    virtual int32_t GetDsdSpeed() const noexcept = 0;

	virtual bool TestDsdFileFormat(const std::wstring& file_path) const = 0;

    virtual bool IsDsdFile() const noexcept = 0;
protected:
    DsdStream() = default;
};

}

