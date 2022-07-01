//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>

#include <base/audioformat.h>
#include <base/align_ptr.h>

#include <stream/stream.h>
#include <stream/idsdstream.h>
#include <stream/filestream.h>

namespace xamp::stream {

class BassFileStream final
	: public FileStream
	, public IDsdStream {
public:
	BassFileStream();

	XAMP_PIMPL(BassFileStream)

    void OpenFile(std::wstring const & file_path) override;

	void Close() noexcept override;

	double GetDuration() const override;

	AudioFormat GetFormat() const noexcept override;

    uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	double GetPosition() const override;

	std::string_view GetDescription() const noexcept override;

	uint8_t GetSampleSize() const noexcept override;

	bool IsDsdFile() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	DsdModes GetDsdMode() const noexcept override;

    uint32_t GetDsdSampleRate() const override;

	DsdFormat GetDsdFormat() const noexcept override;

    void SetDsdToPcmSampleRate(uint32_t sample_rate) override;

    uint32_t GetDsdSpeed() const noexcept override;

	uint32_t GetBitDepth() const override;

	static HashSet<std::string> GetSupportFileExtensions();

	uint32_t GetHStream() const noexcept;

    bool IsActive() const noexcept override;

    bool SupportDOP() const noexcept override;

    bool SupportDOP_AA() const noexcept override;

    bool SupportNativeSD() const noexcept override;
private:	
	class BassFileStreamImpl;
	AlignPtr<BassFileStreamImpl> stream_;
};

}

