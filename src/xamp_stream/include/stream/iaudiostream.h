//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <stream/stream.h>

namespace xamp::stream {

using namespace base;

class XAMP_STREAM_API XAMP_NO_VTABLE IAudioStream {
public:
    XAMP_BASE_CLASS(IAudioStream)

    [[nodiscard]] virtual bool IsFile() const noexcept = 0;

	virtual void Close() noexcept = 0;

    [[nodiscard]] virtual double GetDuration() const = 0;

    virtual uint32_t GetSamples(void *buffer, uint32_t length) const noexcept = 0;

    [[nodiscard]] virtual AudioFormat GetFormat() const noexcept = 0;

    virtual void Seek(double stream_time) const = 0;

    [[nodiscard]] virtual std::string_view GetDescription() const noexcept = 0;
	
    [[nodiscard]] virtual uint8_t GetSampleSize() const noexcept = 0;
protected:
    IAudioStream() = default;
};

}
