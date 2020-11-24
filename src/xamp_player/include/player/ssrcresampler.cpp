#include <player/ssrcresampler.h>

namespace xamp::player {

class SSRCResampler::SSRCResamplerImpl {
public:    
    SSRCResamplerImpl() {
    }

    void Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate) {
    }

    bool Process(float const* samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
        return false;
    }

    void Flush() {
    }
};

const std::string_view SSRCResampler::VERSION = "SSRC ";

SSRCResampler::SSRCResampler()
    : impl_(MakeAlign<SSRCResamplerImpl>()) {
}

SSRCResampler::~SSRCResampler() = default;

void SSRCResampler::Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate) {
    impl_->Start(input_samplerate, num_channels, output_samplerate);
}

std::string_view SSRCResampler::GetDescription() const noexcept {
    return VERSION;
}

bool SSRCResampler::Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
    return impl_->Process(samples, num_sample, buffer);
}

void SSRCResampler::Flush() {
    impl_->Flush();
}

AlignPtr<Resampler> SSRCResampler::Clone() {
    auto other = MakeAlign<Resampler, SSRCResampler>();
    return other;
}

}