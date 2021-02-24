//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <fstream>
#include <filesystem>

#include <base/audioformat.h>
#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API WaveFileWriter final {
public:
	WaveFileWriter() = default;
	
	void Open(std::filesystem::path const& file_path, AudioFormat const& format);

	XAMP_DISABLE_COPY(WaveFileWriter)

	void Close();

	void Write(float const* sample, uint32_t num_samples);
	
private:
	void WriteHeader(AudioFormat const& format);
	
	void WriteDataLength();

	void WriteSample(float sample);

	uint32_t data_length_{0};
	AudioFormat format_;
	std::ofstream file_;
};

}
