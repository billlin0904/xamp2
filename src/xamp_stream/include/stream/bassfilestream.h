//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idsdstream.h>
#include <stream/filestream.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/uuidof.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API BassFileStream final : public FileStream, public IDsdStream {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassFileStream, "E421F2D7-2716-4CB7-9A0F-07B16DE32EBA")

public:
	constexpr static auto Description = std::string_view("BassFileStream");

	BassFileStream();

	XAMP_PIMPL(BassFileStream)

    void OpenFile(Path const& file_path) override;

	void Close() noexcept override;

	XAMP_NO_DISCARD double GetDurationAsSeconds() const override;

	XAMP_NO_DISCARD AudioFormat GetFormat() const override;

	XAMP_NO_DISCARD uint32_t GetSamples(void* buffer, uint32_t length) const override;

	void SeekAsSeconds(double stream_time) const override;

	XAMP_NO_DISCARD std::string_view GetDescription() const noexcept override;

	XAMP_NO_DISCARD uint32_t GetSampleSize() const noexcept override;

	XAMP_NO_DISCARD bool IsDsdFile() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	XAMP_NO_DISCARD DsdModes GetDsdMode() const noexcept override;

	XAMP_NO_DISCARD uint32_t GetDsdSampleRate() const override;

	XAMP_NO_DISCARD DsdFormat GetDsdFormat() const noexcept override;

    void SetDsdToPcmSampleRate(uint32_t sample_rate) override;

	XAMP_NO_DISCARD uint32_t GetDsdSpeed() const override;

	XAMP_NO_DISCARD uint32_t GetBitDepth() const override;

	XAMP_NO_DISCARD uint32_t GetHStream() const noexcept;

	XAMP_NO_DISCARD bool IsActive() const noexcept override;

	XAMP_NO_DISCARD bool SupportDOP() const noexcept override;

	XAMP_NO_DISCARD bool SupportDOP_AA() const noexcept override;

	XAMP_NO_DISCARD bool SupportNativeSD() const noexcept override;

	XAMP_NO_DISCARD Uuid GetTypeId() const override;
private:	
	class BassFileStreamImpl;
	AlignPtr<BassFileStreamImpl> stream_;
};

XAMP_STREAM_NAMESPACE_END

