#include <base/audioformat.h>

namespace xamp::base {

const AudioFormat AudioFormat::kUnknowFormat;
const AudioFormat AudioFormat::kPCM441Khz(DataFormat::FORMAT_PCM,
	2,
	ByteFormat::SINT16, 
	44100);


AudioFormat AudioFormat::ToFloatFormat(AudioFormat const& source_format) noexcept {
	return AudioFormat{
		DataFormat::FORMAT_PCM,
		source_format.GetChannels(),
		ByteFormat::FLOAT32,
		source_format.GetSampleRate(),
		PackedFormat::INTERLEAVED
	};
}

}
