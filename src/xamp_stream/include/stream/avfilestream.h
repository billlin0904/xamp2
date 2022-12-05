//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/uuidof.h>

#include <stream/stream.h>
#include <stream/filestream.h>

namespace xamp::stream {

class XAMP_STREAM_API AvFileStream final : public FileStream {
public:
	AvFileStream();

	XAMP_PIMPL(AvFileStream)

	void OpenFile(const std::wstring& file_path) override;

	void Close() noexcept override;

	double GetDuration() const override;

	AudioFormat GetFormat() const noexcept override;

	uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	std::string_view GetDescription() const noexcept override;

	uint8_t GetSampleSize() const noexcept override;

	bool IsActive() const noexcept override;

	uint32_t GetBitDepth() const override;

private:
	class AvFileStreamImpl;
	AlignPtr<AvFileStreamImpl> impl_;
};
XAMP_MAKE_CLASS_UUID(AvFileStream, "D59756C0-19CE-4E4B-BC83-6616BBB2930B")

}

