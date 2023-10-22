//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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

	void OpenFile(const Path & file_path) override;

	void Close() noexcept override;

	[[nodiscard]] double GetDurationAsSeconds() const override;

	[[nodiscard]] AudioFormat GetFormat() const noexcept override;

	uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void SeekAsSeconds(double stream_time) const override;

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	[[nodiscard]] uint32_t GetSampleSize() const noexcept override;

	[[nodiscard]] bool IsActive() const noexcept override;

	[[nodiscard]] uint32_t GetBitDepth() const override;

	[[nodiscard]] int64_t GetBitRate() const;

	[[nodiscard]] Uuid GetTypeId() const override;
private:
	class AvFileStreamImpl;
	AlignPtr<AvFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

