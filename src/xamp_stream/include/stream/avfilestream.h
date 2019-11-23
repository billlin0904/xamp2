//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>

#include <stream/stream.h>
#include <stream/audiostream.h>
#include <stream/filestream.h>

namespace xamp::stream {

using namespace base;

#if ENABLE_FFMPEG

class XAMP_STREAM_API AvFileStream final : public FileStream {
public:
	AvFileStream();

	XAMP_PIMPL(AvFileStream)

    void OpenFromFile(const std::wstring& file_path) override;

	void Close() override;

	double GetDuration() const override;

	AudioFormat GetFormat() const noexcept override;

	int32_t GetSamples(void* buffer, int32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	std::string_view GetDescription() const noexcept override;

	int32_t GetSampleSize() const noexcept override;
private:
	class AvFileStreamImpl;
	AlignPtr<AvFileStreamImpl> impl_;
};

#endif

}
