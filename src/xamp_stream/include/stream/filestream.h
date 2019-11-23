//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/audiostream.h>

namespace xamp::stream {

class XAMP_STREAM_API FileStream : public AudioStream {
public:
    XAMP_BASE_CLASS(FileStream)	

	bool IsFile() const noexcept override {
		return true;
	}

    virtual void OpenFromFile(const std::wstring & file_path) = 0;	

protected:
    FileStream() = default;
};

}