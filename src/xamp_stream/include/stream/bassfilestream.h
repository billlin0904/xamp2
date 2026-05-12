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

	BassFileStream();

	XAMP_PIMPL(BassFileStream)

    void OpenFile(const Path& file_path) override;

	void Open(ArchiveEntry archive_entry) override;

	void SetRate(float rate = 0.0f) override;

	void Close() override;

	// Check if the stream is at the end.
	// If read CD or use BASS_ASYNCFILE flag, Must use this function to check.
	bool EndOfStream() const;

	[[nodiscard]] double GetDuration() const override;

	[[nodiscard]] AudioFormat GetFormat() const override;

	[[nodiscard]] uint32_t GetSamples(void* buffer, uint32_t length) const override;

	void Seek(double stream_time) const override;

	[[nodiscard]] uint32_t GetSampleSize() const override;

	[[nodiscard]] bool IsDsdFile() const override;

	void SetDSDMode(DsdModes mode) override;

	[[nodiscard]] DsdModes GetDsdMode() const override;

	[[nodiscard]] uint32_t GetDsdSampleRate() const override;

	[[nodiscard]] DsdFormat GetDsdFormat() const override;

    void SetDsdToPcmSampleRate(uint32_t sample_rate) override;

	[[nodiscard]] uint32_t GetDsdSpeed() const override;

	[[nodiscard]] uint32_t GetBitDepth() const override;

	[[nodiscard]] uint32_t GetBitRate() const override;

	[[nodiscard]] uint32_t GetHStream() const ;

	[[nodiscard]] bool IsActive() const override;

	[[nodiscard]] bool SupportDOP() const override;

	[[nodiscard]] bool SupportDOP_AA() const override;

	[[nodiscard]] bool SupportNativeSD() const override;

private:	
	class BassFileStreamImpl;
	ScopedPtr<BassFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

