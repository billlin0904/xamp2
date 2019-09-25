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

class XAMP_STREAM_API BassFileStream 
	: public FileStream
	, public DSDStream {
public:
	BassFileStream();

	XAMP_PIMPL(BassFileStream)

	void OpenFromFile(const std::wstring & file_path, OpenMode open_mode) override;

	void Close() override;

	double GetDuration() const override;

	AudioFormat GetFormat() const override;

	int32_t GetSamples(void* buffer, int32_t length) const noexcept override;

	void Seek(double stream_time) const override;

	std::wstring GetStreamName() const override;

	int32_t GetSampleSize() const override;

	bool IsDSDFile() const override;

	bool SupportDOP() const override;

	bool SupportDOP_AA() const override;

	bool SupportRAW() const override;

	void SetDSDMode(DSDModes mode) override;

	DSDModes GetDSDMode() const override;

	int32_t GetDSDSampleRate() const override;

	DSDSampleFormat GetDSDSampleFormat() const override;

	bool SetPCMSampleRate(int32_t samplerate) const override;
private:
	class BassFileStreamImpl;
	AlignPtr<BassFileStreamImpl> stream_;
};

}

