//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idsdstream.h>
#include <stream/filestream.h>
#include <stream/iaudioprocessor.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/uuidof.h>
#include <base/archivefile.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API BassFileStream final : public FileStream, public IDsdStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassFileStream, "E421F2D7-2716-4CB7-9A0F-07B16DE32EBA")

public:
	XAMP_DECLARE_UUID_CLASS(BassFileStream)

	explicit BassFileStream(float rate = 0.0f);

	XAMP_PIMPL(BassFileStream)

    void openFile(const Path& file_path) override;

	void open(ArchiveEntry archive_entry) override;

	void close() override;

	// Check if the stream is at the end.
	// If read CD or use BASS_ASYNCFILE flag, Must use this function to check.
	bool endOfStream() const override;

	[[nodiscard]] double getDuration() const override;

	[[nodiscard]] AudioFormat getFormat() const override;

	[[nodiscard]] uint32_t getSamples(void* buffer, uint32_t length) const override;

	void seek(double stream_time) const override;

	[[nodiscard]] uint32_t getSampleSize() const override;

	[[nodiscard]] bool isDsdFile() const override;

	void setDSDMode(DsdModes mode) override;

	[[nodiscard]] DsdModes getDsdMode() const override;

	[[nodiscard]] uint32_t getDsdSampleRate() const override;

	[[nodiscard]] DsdFormat getDsdFormat() const override;

    void setDsdToPcmSampleRate(uint32_t sample_rate) override;

	[[nodiscard]] uint32_t getDsdSpeed() const override;

	[[nodiscard]] uint32_t getBitDepth() const override;

	[[nodiscard]] uint32_t getBitRate() const override;

	[[nodiscard]] uint32_t getHStream() const ;

	[[nodiscard]] bool isActive() const override;

	[[nodiscard]] bool supportDOP() const override;

	[[nodiscard]] bool supportDOP_AA() const override;

	[[nodiscard]] bool supportNativeSD() const override;

private:	
	class BassFileStreamImpl;
	ScopedPtr<BassFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

