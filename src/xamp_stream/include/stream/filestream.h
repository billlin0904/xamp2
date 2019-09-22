//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/audiostream.h>

namespace xamp::stream {

enum OpenMode {
	IN_MEMORY,
	NOT_IN_MEMORY
};

class XAMP_STREAM_API FileStream : public AudioStream {
public:
    XAMP_BASE_CLASS(FileStream)	

	bool IsFile() const override {
		return true;
	}

    virtual void OpenFromFile(const std::wstring & file_path, OpenMode open_mode) = 0;

	virtual bool IsDSDFile() const = 0;
protected:
    FileStream() = default;
};

}