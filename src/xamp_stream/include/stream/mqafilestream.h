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

class XAMP_STREAM_API MqaIdentifier {
public:
	explicit MqaIdentifier(const Path& file_path);

	bool Detect();

	bool IsMQA() const;

	bool IsMQAStudio() const;

	uint32_t GetOriginalSampleRate() const;

	XAMP_PIMPL(MqaIdentifier)
private:
	class MqaIdentifierImpl;
	ScopedPtr<MqaIdentifierImpl> impl_;
};

class XAMP_STREAM_API MqaFileStream final : public FileStream, public IDsdStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(MqaFileStream, "D7F9B925-CFCC-4A86-95EC-6091D779FAB3")

public:
	XAMP_DECLARE_UUID_CLASS(MqaFileStream)

	MqaFileStream();

	XAMP_PIMPL(MqaFileStream)

	void OpenFile(const Path& file_path, float rate = 0.0f) override;

	void Open(ArchiveEntry archive_entry, float rate = 0.0f) override;

	void Close() noexcept override;

	[[nodiscard]] double GetDuration() const override;

	[[nodiscard]] AudioFormat GetFormat() const override;

	[[nodiscard]] uint32_t GetSamples(void* buffer, uint32_t length) const override;

	void Seek(double stream_time) const override;

	[[nodiscard]] uint32_t GetSampleSize() const noexcept override;

	[[nodiscard]] bool IsDsdFile() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	[[nodiscard]] DsdModes GetDsdMode() const noexcept override;

	[[nodiscard]] uint32_t GetDsdSampleRate() const override;

	[[nodiscard]] DsdFormat GetDsdFormat() const noexcept override;

	void SetDsdToPcmSampleRate(uint32_t sample_rate) override;

	[[nodiscard]] uint32_t GetDsdSpeed() const override;

	[[nodiscard]] uint32_t GetBitDepth() const override;

	[[nodiscard]] uint32_t GetBitRate() const override;

	[[nodiscard]] bool IsActive() const noexcept override;

	[[nodiscard]] bool SupportDOP() const noexcept override;

	[[nodiscard]] bool SupportDOP_AA() const noexcept override;

	[[nodiscard]] bool SupportNativeSD() const noexcept override;

private:
	class MqaFileStreamImpl;
	ScopedPtr<MqaFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END