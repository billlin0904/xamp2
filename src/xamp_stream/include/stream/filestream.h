//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <stream/stream.h>
#include <stream/iaudiostream.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE FileStream : public IAudioStream {
public:
    XAMP_BASE_CLASS(FileStream)

	bool IsFile() const noexcept override {
		return true;
	}

    virtual void OpenFile(std::wstring const & file_path) = 0;

    virtual int32_t GetBitDepth() const = 0;

    virtual bool IsActive() const noexcept = 0;

protected:
    FileStream() = default;
};

}
