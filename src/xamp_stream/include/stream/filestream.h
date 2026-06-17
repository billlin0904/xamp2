//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudiostream.h>
#include <base/archivefile.h>
#include <base/fs.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API XAMP_NO_VTABLE FileStream : public IAudioStream {
public:
    XAMP_BASE_CLASS(FileStream)

    [[nodiscard]] bool isFile() const override {
		return true;
	}

    virtual void openFile(const Path & file_path) = 0;

    virtual void open(ArchiveEntry archive_entry) = 0;

    virtual bool endOfStream() const = 0;
protected:
    FileStream() = default;
};

XAMP_STREAM_NAMESPACE_END
