#include <base/exception.h>
#include <base/memory.h>
#include <base/dataconverter.h>
#include <stream/wavefilewriter.h>

namespace xamp::stream {

struct CHUNK {
	char id[4]{ 0 };
	uint32_t size{ 0 };
};

struct RIFF_HEADER {
	CHUNK descriptor;
	char type[4]{ 0 };
};

struct WAVE_HEADER {
	CHUNK descriptor;
	uint16_t audio_format{ 0 };
	uint16_t num_channels{ 0 };
	uint32_t sample_rate{ 0 };
	uint32_t byte_rate{ 0 };
	uint16_t block_align{ 0 };
	uint16_t bits_per_sample{0};
};

struct DATA_HEADER {
	CHUNK descriptor;
};

struct LIST {
	CHUNK descriptor;
	char type_id[4]{ 0 };
};

struct COMBINED_HEADER {
	RIFF_HEADER riff;
	WAVE_HEADER wave;
	DATA_HEADER data;
};

void WaveFileWriter::Close() {
	if (data_length_ == 0) {
		file_.close();
		return;
	}
	WriteDataLength();
	file_.close();
	data_length_ = 0;
}

void WaveFileWriter::Open(Path const& file_path, AudioFormat const& format) {
	if (format.GetBitsPerSample() != 16 && format.GetBitsPerSample() != 24) {
		throw NotSupportFormatException();
	}
	Close();
	file_.open(file_path, std::ios::binary);
	if (!file_) {
		throw PlatformSpecException();
	}
	WriteHeader(format);	
}

bool WaveFileWriter::TryWrite(float const* sample, size_t num_samples) {
	try {
		Write(sample, num_samples);
	}
	catch (...) {
		return false;
	}
	return true;
}

void WaveFileWriter::Write(float const* sample, size_t num_samples) {
	for (size_t i = 0; i < num_samples; ++i) {
		WriteSample(sample[i]);
	}
}

void WaveFileWriter::WriteSample(float sample) {
	if (format_.GetBitsPerSample() == 16) {
		auto output = static_cast<int16_t>(kFloat16Scale * sample);
		if (!file_.write(reinterpret_cast<const char*>(&output), sizeof(output))) {
			throw PlatformSpecException();
		}
		data_length_ += 2;
	}
	else if (format_.GetBitsPerSample() == 24) {
		const Int24 output(sample);
		if (!file_.write(reinterpret_cast<const char*>(output.data.data()), output.data.size())) {
			throw PlatformSpecException();
		}
		data_length_ += 3;
	} else {
		throw NotSupportFormatException();
	}
}

void WaveFileWriter::WriteDataLength() {
	if (!file_.seekp(4)) {
		throw PlatformSpecException();
	}
	uint32_t length = data_length_ + sizeof(COMBINED_HEADER) - 8;
	if (!file_.write(reinterpret_cast<const char*>(&length), sizeof(length))) {
		throw PlatformSpecException();
	}
	if (!file_.seekp(40)) {
		throw PlatformSpecException();
	}
	if (!file_.write(reinterpret_cast<const char*>(&data_length_), sizeof(data_length_))) {
		throw PlatformSpecException();
	}
}

void WaveFileWriter::WriteHeader(AudioFormat const& format) {
	COMBINED_HEADER header;
	MemoryCopy(header.riff.descriptor.id, "RIFF", 4);
	MemoryCopy(header.riff.type, "WAVE", 4);
	MemoryCopy(header.wave.descriptor.id, "fmt ", 4);
	header.wave.descriptor.size = 16;
	header.wave.audio_format = 1; // 1: WAVE_FORMAT_PCM
	header.wave.num_channels = format.GetChannels();
	header.wave.sample_rate = format.GetSampleRate();
	header.wave.byte_rate = format.GetSampleRate() * format.GetChannels() * format.GetBytesPerSample();
	header.wave.block_align = static_cast<uint16_t>(format.GetChannels() * format.GetBytesPerSample());
	header.wave.bits_per_sample = static_cast<uint16_t>(format.GetBitsPerSample());
	MemoryCopy(header.data.descriptor.id, "data", 4);
	header.data.descriptor.size = 0;
	if (!file_.write(reinterpret_cast<const char*>(&header), sizeof(COMBINED_HEADER))) {
		throw PlatformSpecException();
	}
	format_ = format;
}

}
