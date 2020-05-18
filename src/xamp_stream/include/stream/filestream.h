//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/audiostream.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE FileStream : public AudioStream {
public:
	bool IsFile() const noexcept override {
		return true;
	}

    virtual void OpenFromFile(std::wstring const & file_path) = 0;

protected:
    FileStream() = default;
};

}
