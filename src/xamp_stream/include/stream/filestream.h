//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudiostream.h>

#include <base/fs.h>

XAMP_STREAM_NAMESPACE_BEGIN

/*
* FileStream is a base class for all file stream.
* 
*/
class XAMP_STREAM_API XAMP_NO_VTABLE FileStream : public IAudioStream {
public:
    XAMP_BASE_CLASS(FileStream)

    /*
    * IsFile return true.
    * 
    */
    [[nodiscard]] bool IsFile() const noexcept override {
		return true;
	}

    /*
    * OpenFile open a file.
    * 
    * @param file_path: the file path.
    */
    virtual void OpenFile(const Path & file_path) = 0;

protected:
    FileStream() = default;
};

XAMP_STREAM_NAMESPACE_END
