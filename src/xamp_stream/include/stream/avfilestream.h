//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>

#include <stream/stream.h>
#include <stream/filestream.h>

namespace xamp::stream {

#if ENABLE_FFMPEG

class XAMP_STREAM_API AvFileStream final : public FileStream {
public:
	AvFileStream();

    ~AvFileStream() override;

    void OpenFromFile(const std::wstring& file_path) override;

	void Close() override;

	[[nodiscard]] double GetDuration() const override;

	[[nodiscard]] AudioFormat GetFormat() const noexcept override;

    uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	[[nodiscard]] uint32_t GetSampleSize() const noexcept override;
private:
	class AvFileStreamImpl;
    AlignPtr<AvFileStreamImpl> impl_;
};

#endif

}
