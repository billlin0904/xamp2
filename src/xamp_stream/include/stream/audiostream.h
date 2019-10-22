//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <stream/stream.h>

namespace xamp::stream {

using namespace base;

class XAMP_STREAM_API XAMP_NO_VTABLE AudioStream {
public:
	XAMP_BASE_CLASS(AudioStream)

	virtual bool IsFile() const noexcept = 0;

	virtual void Close() = 0;

    virtual double GetDuration() const = 0;    

    virtual int32_t GetSamples(void *buffer, int32_t length) const noexcept = 0;

	virtual AudioFormat GetFormat() const = 0;

    virtual void Seek(double stream_time) const = 0;

    virtual std::string GetStreamName() const = 0;
	
	virtual int32_t GetSampleSize() const = 0;
protected:
    AudioStream() = default;
};

}

