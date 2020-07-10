#include <base/exception.h>
#include <player/nullresampler.h>

namespace xamp::player {

#define CheckBufferFlow(expr) \
    do {\
        if (!(expr)) {\
        throw Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, "Buffer overflow.");\
        }\
    } while(false)

NullResampler::NullResampler(DsdModes dsd_mode, uint32_t sample_size)
    : dsd_mode_(dsd_mode)
    , sample_size_(sample_size)
    , process_(nullptr) {
    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        process_ = &NullResampler::ProcessNativeDsd;
    }
    else {
        process_ = &NullResampler::ProcessPcm;
    }
}

void NullResampler::Start(uint32_t, uint32_t, uint32_t, uint32_t) {
}

bool NullResampler::Process(float const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
    return (*this.*process_)(reinterpret_cast<int8_t const*>(sample_buffer), num_samples, buffer);
}

std::string_view NullResampler::GetDescription() const noexcept {
    return "None";
}

void NullResampler::Flush() {
}

bool NullResampler::ProcessNativeDsd(int8_t const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
    CheckBufferFlow(buffer.TryWrite(sample_buffer, num_samples));
    return true;
}

bool NullResampler::ProcessPcm(int8_t const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
    CheckBufferFlow(buffer.TryWrite(sample_buffer, num_samples * sample_size_));
    return true;
}

}
