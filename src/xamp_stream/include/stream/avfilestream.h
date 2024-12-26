//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/filestream.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/uuidof.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API AvFileStream final : public FileStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(AvFileStream, "D59756C0-19CE-4E4B-BC83-6616BBB2930B")

public:
	constexpr static auto Description = std::string_view("AvFileStream");

	AvFileStream();

	XAMP_PIMPL(AvFileStream)

	void OpenFile(const Path & file_path) override;

	void Close() noexcept override;

	XAMP_NO_DISCARD double GetDurationAsSeconds() const override;

	XAMP_NO_DISCARD AudioFormat GetFormat() const noexcept override;

	uint32_t GetSamples(void* buffer, uint32_t length) const override;

	void SeekAsSeconds(double stream_time) const override;

	XAMP_NO_DISCARD std::string_view GetDescription() const noexcept override;

	XAMP_NO_DISCARD uint32_t GetSampleSize() const noexcept override;

	XAMP_NO_DISCARD bool IsActive() const noexcept override;

	XAMP_NO_DISCARD uint32_t GetBitDepth() const override;

	XAMP_NO_DISCARD int64_t GetBitRate() const;

	XAMP_NO_DISCARD Uuid GetTypeId() const override;
private:
	class AvFileStreamImpl;
	ScopedPtr<AvFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

