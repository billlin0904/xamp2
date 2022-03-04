//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <fstream>
#include <filesystem>
#include <map>

#include <base/audioformat.h>
#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API WaveFileWriter final {
public:
	WaveFileWriter() = default;
	
	void Open(Path const& file_path, AudioFormat const& format);

	XAMP_DISABLE_COPY(WaveFileWriter)

	void Close();

	bool TryWrite(float const* sample, size_t num_samples);

	void Write(float const* sample, size_t num_samples);
	
private:
	void WriteHeader(AudioFormat const& format);
	
	void WriteDataLength();

	void WriteSample(float sample);

	uint32_t data_length_{0};
	AudioFormat format_;
	std::ofstream file_;
};

}
