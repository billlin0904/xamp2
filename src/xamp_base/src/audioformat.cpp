#include <base/stl.h>
#include <base/audioformat.h>

#include <sstream>

namespace xamp::base {

#define DECLARE_AUDIO_FORMAT_IMPL(Name, SampleRate) \
	const AudioFormat AudioFormat::k16Bit##Name(DataFormat::FORMAT_PCM,\
		2,\
		ByteFormat::SINT16,\
		SampleRate);\
	const AudioFormat AudioFormat::k24Bit##Name(DataFormat::FORMAT_PCM,\
		2, \
		ByteFormat::SINT24,\
		SampleRate);\
	const AudioFormat AudioFormat::kFloat##Name(DataFormat::FORMAT_PCM,\
		2, \
		ByteFormat::FLOAT32,\
		SampleRate)

DECLARE_AUDIO_FORMAT_IMPL(PCM441Khz, 44100);
DECLARE_AUDIO_FORMAT_IMPL(PCM48Khz,  48000);
DECLARE_AUDIO_FORMAT_IMPL(PCM96Khz,  96000);
DECLARE_AUDIO_FORMAT_IMPL(PCM882Khz, 88200);
DECLARE_AUDIO_FORMAT_IMPL(PCM1764Khz,176400);
DECLARE_AUDIO_FORMAT_IMPL(PCM192Khz, 192000);
DECLARE_AUDIO_FORMAT_IMPL(PCM3528Khz,352800);
DECLARE_AUDIO_FORMAT_IMPL(PCM384Khz, 384000);
DECLARE_AUDIO_FORMAT_IMPL(PCM768Khz, 768000);

const AudioFormat AudioFormat::kUnknownFormat;

AudioFormat AudioFormat::ToFloatFormat(AudioFormat const& source_format) noexcept {
	return AudioFormat{
		DataFormat::FORMAT_PCM,
		source_format.GetChannels(),
		ByteFormat::FLOAT32,
		source_format.GetSampleRate(),
		PackedFormat::INTERLEAVED
	};
}

std::string AudioFormat::ToString() const {
	std::ostringstream ostr;
	ostr << *this;
	return ostr.str();
}

std::string AudioFormat::ToShortString() const {
	std::ostringstream ostr;

    ostr << GetChannels() << "Ch/" <<  GetBitsPerSample() << "bit/";

	if (GetSampleRate() % 1000 > 0) {
		ostr << std::fixed << std::setprecision(1) << static_cast<float>(GetSampleRate()) / 1000.0f << " Khz";
	}
	else {
		ostr << GetSampleRate() / 1000 << " Khz";
	}
	return ostr.str();
}

size_t AudioFormat::GetHash() const {	
	return std::hash<std::string>{}(ToString());
}

}
