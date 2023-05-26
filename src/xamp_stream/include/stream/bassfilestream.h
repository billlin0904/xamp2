//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idsdstream.h>
#include <stream/filestream.h>

#include <base/audioformat.h>
#include <base/align_ptr.h>
#include <base/uuidof.h>
#include <base/pimplptr.h>

namespace xamp::stream {

class XAMP_STREAM_API BassFileStream final : public FileStream, public IDsdStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassFileStream, "E421F2D7-2716-4CB7-9A0F-07B16DE32EBA")

public:
	BassFileStream();

	XAMP_PIMPL(BassFileStream)

    void OpenFile(Path const& file_path) override;

	void Close() noexcept override;

	double GetDurationAsSeconds() const override;

	AudioFormat GetFormat() const override;

    uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void SeekAsSeconds(double stream_time) const override;

	std::string_view GetDescription() const noexcept override;

	uint8_t GetSampleSize() const noexcept override;

	bool IsDsdFile() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	DsdModes GetDsdMode() const noexcept override;

    uint32_t GetDsdSampleRate() const override;

	DsdFormat GetDsdFormat() const noexcept override;

    void SetDsdToPcmSampleRate(uint32_t sample_rate) override;

    uint32_t GetDsdSpeed() const override;

	uint32_t GetBitDepth() const override;

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

