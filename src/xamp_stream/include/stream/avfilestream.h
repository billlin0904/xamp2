//=====================================================================================================================
// Copyright (c) 2018-2022 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/filestream.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/uuidof.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API AvFileStream final : public FileStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(AvFileStream, "D59756C0-19CE-4E4B-BC83-6616BBB2930B")

public:
	AvFileStream();

	XAMP_PIMPL(AvFileStream)

	void OpenFile(Path const& file_path) override;

	void Close() noexcept override;

	double GetDurationAsSeconds() const override;

	AudioFormat GetFormat() const noexcept override;

	uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void SeekAsSeconds(double stream_time) const override;

	std::string_view GetDescription() const noexcept override;

	uint8_t GetSampleSize() const noexcept override;

	bool IsActive() const noexcept override;

	uint32_t GetBitDepth() const override;

	int64_t GetBitRate() const;
private:
	class AvFileStreamImpl;
	PimplPtr<AvFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

