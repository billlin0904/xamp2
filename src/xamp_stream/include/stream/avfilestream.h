//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <set>

#include <base/stl.h>
#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>

#include <stream/stream.h>
#include <stream/filestream.h>

namespace xamp::stream {

class AvFileStream final : public FileStream {
public:
	AvFileStream();

	XAMP_PIMPL(AvFileStream)

    void OpenFile(const std::wstring& file_path) override;

	void Close() noexcept override;

	[[nodiscard]] double GetDuration() const override;

	[[nodiscard]] AudioFormat GetFormat() const noexcept override;

    uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	[[nodiscard]] uint8_t GetSampleSize() const noexcept override;

	static std::set<std::string> GetSupportFileExtensions();
private:
	class AvFileStreamImpl;
    AlignPtr<AvFileStreamImpl> impl_;
};

}
