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

	bool endOfStream() const override;

    void setIntegerPcm(bool enabled);
    std::optional<xamp::pcm::Format> integerPcmFormat() const override;
	void useCustomIOContext(bool enable);

	void openFile(const Path& file_path) override;

	void open(ArchiveEntry archive_entry) override;

	void close() override;

	[[nodiscard]] double getDuration() const override;

	[[nodiscard]] AudioFormat getFormat() const override;

	void seek(double stream_time) const override;

	[[nodiscard]] uint32_t getSamples(void* buffer, uint32_t length) const override;

	[[nodiscard]] uint32_t getSampleSize() const override;

	[[nodiscard]] bool isActive() const override;

	[[nodiscard]] uint32_t getBitDepth() const override;

	[[nodiscard]] uint32_t getBitRate() const override;

private:
	class AvLibFileStreamImpl;
	ScopedPtr<AvLibFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
