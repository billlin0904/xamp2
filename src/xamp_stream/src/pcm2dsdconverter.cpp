#include <base/memory.h>
#include <stream/fftwlib.h>
#include <stream/pcm2dsdconverter.h>

namespace xamp::stream {

Pcm2DsdConverter::Pcm2DsdConverter(uint32_t output_sample_rate, uint32_t dsd_times) {
}

std::vector<uint8_t> Pcm2DsdConverter::ProcessChannel(std::vector<double> &buffer, uint32_t sample_size, uint32_t sample_rate) {
    return std::vector<uint8_t>();
}

[[nodiscard]] std::string_view Pcm2DsdConverter::GetDescription() const noexcept {
	return "";
}

[[nodiscard]] bool Pcm2DsdConverter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
	return true;
}

bool Pcm2DsdConverter::Process(float const* samples, size_t num_sample, AudioBuffer<int8_t>& buffer) {
	return false;
}

}
