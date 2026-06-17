//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/mqaidentifier.h>
#include <stream/idsdstream.h>
#include <stream/filestream.h>
#include <stream/iaudioprocessor.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/uuidof.h>
#include <base/archivefile.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API MqaFileStream final : public FileStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(MqaFileStream, "D7F9B925-CFCC-4A86-95EC-6091D779FAB3")

public:
	XAMP_DECLARE_UUID_CLASS(MqaFileStream)

	MqaFileStream();

	XAMP_PIMPL(MqaFileStream)

	void openFile(const Path& file_path) override;

	void open(ArchiveEntry archive_entry) override;

	void close() override;

	bool endOfStream() const override;

	[[nodiscard]] double getDuration() const override;

	[[nodiscard]] AudioFormat getFormat() const override;

	[[nodiscard]] uint32_t getSamples(void* buffer, uint32_t length) const override;

	void seek(double stream_time) const override;

	[[nodiscard]] uint32_t getSampleSize() const override;

	[[nodiscard]] uint32_t getBitDepth() const override;

	[[nodiscard]] uint32_t getBitRate() const override;

	[[nodiscard]] bool isActive() const override;

private:
	class MqaFileStreamImpl;
	ScopedPtr<MqaFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
