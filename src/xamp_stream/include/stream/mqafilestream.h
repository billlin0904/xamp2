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

class XAMP_STREAM_API MqaFileStream final : public FileStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(MqaFileStream, "D7F9B925-CFCC-4A86-95EC-6091D779FAB3")

public:
	XAMP_DECLARE_UUID_CLASS(MqaFileStream)

	MqaFileStream();

	XAMP_PIMPL(MqaFileStream)

	void OpenFile(const Path& file_path) override;

	void Open(ArchiveEntry archive_entry) override;

	void Close() noexcept override;

	void SetRate(float rate = 0.0f) override;

	[[nodiscard]] double GetDuration() const override;

	[[nodiscard]] AudioFormat GetFormat() const override;

	[[nodiscard]] uint32_t GetSamples(void* buffer, uint32_t length) const override;

	void Seek(double stream_time) const override;

	[[nodiscard]] uint32_t GetSampleSize() const noexcept override;

	[[nodiscard]] uint32_t GetBitDepth() const override;

	[[nodiscard]] uint32_t GetBitRate() const override;

	[[nodiscard]] bool IsActive() const noexcept override;

private:
	class MqaFileStreamImpl;
	ScopedPtr<MqaFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END