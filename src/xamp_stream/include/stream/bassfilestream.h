//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idsdstream.h>
#include <stream/filestream.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/uuidof.h>
#include <base/archivefile.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API BassFileStream final : public FileStream, public IDsdStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassFileStream, "E421F2D7-2716-4CB7-9A0F-07B16DE32EBA")

public:
	constexpr static auto Description = std::string_view("BassFileStream");

	BassFileStream();

	XAMP_PIMPL(BassFileStream)

    void OpenFile(const Path& file_path) override;

	void Open(ArchiveEntry archive_entry) override;

	void Close() noexcept override;

	// Check if the stream is at the end.
	// If read CD or use BASS_ASYNCFILE flag, Must use this function to check.
	bool EndOfStream() const;

	[[nodiscard]] double GetDurationAsSeconds() const override;

	[[nodiscard]] AudioFormat GetFormat() const override;

	[[nodiscard]] uint32_t GetSamples(void* buffer, uint32_t length) const override;

	void SeekAsSeconds(double stream_time) const override;

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	[[nodiscard]] uint32_t GetSampleSize() const noexcept override;

	[[nodiscard]] bool IsDsdFile() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	[[nodiscard]] DsdModes GetDsdMode() const noexcept override;

	[[nodiscard]] uint32_t GetDsdSampleRate() const override;

	[[nodiscard]] DsdFormat GetDsdFormat() const noexcept override;

    void SetDsdToPcmSampleRate(uint32_t sample_rate) override;

	[[nodiscard]] uint32_t GetDsdSpeed() const override;

	[[nodiscard]] uint32_t GetBitDepth() const override;

	[[nodiscard]] uint32_t GetHStream() const noexcept;

	[[nodiscard]] bool IsActive() const noexcept override;

	[[nodiscard]] bool SupportDOP() const noexcept override;

	[[nodiscard]] bool SupportDOP_AA() const noexcept override;

	[[nodiscard]] bool SupportNativeSD() const noexcept override;

	[[nodiscard]] Uuid GetTypeId() const override;
private:	
	class BassFileStreamImpl;
	ScopedPtr<BassFileStreamImpl> stream_;
};

XAMP_STREAM_NAMESPACE_END

