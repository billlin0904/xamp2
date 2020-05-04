//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <base/align_ptr.h>

#include <stream/stream.h>
#include <stream/dsdstream.h>
#include <stream/filestream.h>

namespace xamp::stream {

class XAMP_STREAM_API BassFileStream final
	: public FileStream
	, public DsdStream {
public:
	BassFileStream();

    ~BassFileStream() override;

	static void LoadBassLib();

	void OpenFromFile(const std::wstring & file_path) override;

	void Close() override;

	double GetDuration() const override;

	AudioFormat GetFormat() const noexcept override;

    uint32_t GetSamples(void* buffer, uint32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	std::string_view GetDescription() const noexcept override;

    uint32_t GetSampleSize() const noexcept override;

	bool IsDsdFile() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	DsdModes GetDsdMode() const noexcept override;

    uint32_t GetDsdSampleRate() const override;

	DsdFormat GetDsdFormat() const noexcept override;

    void SetDsdToPcmSampleRate(uint32_t samplerate) override;

    uint32_t GetDsdSpeed() const noexcept override;
private:
	class BassFileStreamImpl;
	align_ptr<BassFileStreamImpl> stream_;
};

}

