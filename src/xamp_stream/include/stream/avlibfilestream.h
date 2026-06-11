//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/filestream.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API AvLibFileStream final : public FileStream {
public:
	XAMP_DECLARE_MAKE_CLASS_UUID(AvLibFileStream, "D59B2D1C-060A-4B4F-8A8E-8FDD0E7F5E47")

	XAMP_DECLARE_UUID_CLASS(AvLibFileStream)

	AvLibFileStream();

	XAMP_PIMPL(AvLibFileStream)

	bool EndOfStream() const override;

	void OpenFile(const Path& file_path) override;

	void Open(ArchiveEntry archive_entry) override;

	void Close() override;

	[[nodiscard]] double GetDuration() const override;

	[[nodiscard]] AudioFormat GetFormat() const override;

	void Seek(double stream_time) const override;

	[[nodiscard]] uint32_t GetSamples(void* buffer, uint32_t length) const override;

	[[nodiscard]] uint32_t GetSampleSize() const override;

	[[nodiscard]] bool IsActive() const override;

	[[nodiscard]] uint32_t GetBitDepth() const override;

	[[nodiscard]] uint32_t GetBitRate() const override;

private:
	class AvLibFileStreamImpl;
	ScopedPtr<AvLibFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
