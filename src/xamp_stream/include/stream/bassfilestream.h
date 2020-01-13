//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>

#include <stream/stream.h>
#include <stream/dsdstream.h>
#include <stream/audiostream.h>
#include <stream/filestream.h>

namespace xamp::stream {

class XAMP_STREAM_API BassFileStream final
	: public FileStream
	, public DsdStream {
public:
	BassFileStream();

	XAMP_PIMPL(BassFileStream)

	static void LoadBassLib();

	void OpenFromFile(const std::wstring & file_path) override;

	void Close() override;

	double GetDuration() const override;

	AudioFormat GetFormat() const noexcept override;

	int32_t GetSamples(void* buffer, int32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	std::string_view GetDescription() const noexcept override;

	int32_t GetSampleSize() const noexcept override;

	bool IsDsdFile() const noexcept override;

    DsdModes GetSupportDsdMode() const noexcept override;

	void SetDSDMode(DsdModes mode) noexcept override;

	DsdModes GetDsdMode() const noexcept override;

	int32_t GetDsdSampleRate() const override;

	DsdSampleFormat GetDsdSampleFormat() const noexcept override;

	void SetPCMSampleRate(int32_t samplerate) override;

    int32_t GetDsdSpeed() const override;
private:
	class BassFileStreamImpl;
	AlignPtr<BassFileStreamImpl> stream_;
};

}

