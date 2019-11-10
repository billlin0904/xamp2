//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
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

using namespace base;

class XAMP_STREAM_API BassFileStream final
	: public FileStream
	, public DSDStream {
public:
	BassFileStream();

	XAMP_PIMPL(BassFileStream)

	void OpenFromFile(const std::wstring & file_path) override;

	void Close() override;

	double GetDuration() const override;

	AudioFormat GetFormat() const noexcept override;

	int32_t GetSamples(void* buffer, int32_t length) const noexcept override;

	void Seek(double stream_time) const override;

    std::string GetStreamName() const noexcept override;

	int32_t GetSampleSize() const noexcept override;

	bool IsDSDFile() const noexcept override;

	bool SupportDOP() const noexcept override;

	bool SupportDOP_AA() const noexcept override;

	bool SupportRAW() const noexcept override;

	void SetDSDMode(DSDModes mode) noexcept override;

	DSDModes GetDSDMode() const noexcept override;

	int32_t GetDSDSampleRate() const override;

	DSDSampleFormat GetDSDSampleFormat() const noexcept override;

	void SetPCMSampleRate(int32_t samplerate) override;

    int32_t GetDSDSpeed() const override;
private:
	class BassFileStreamImpl;
	AlignPtr<BassFileStreamImpl> stream_;
};

}

