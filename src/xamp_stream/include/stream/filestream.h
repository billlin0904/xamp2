//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudiostream.h>

#include <base/fs.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE FileStream : public IAudioStream {
public:
    XAMP_BASE_CLASS(FileStream)

    [[nodiscard]] bool IsFile() const noexcept override {
		return true;
	}

    virtual void OpenFile(Path const & file_path) = 0;

    [[nodiscard]] virtual uint32_t GetBitDepth() const = 0;

protected:
    FileStream() = default;
};

}
