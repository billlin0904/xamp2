#include <base/exception.h>
#include <base/dataconverter.h>
#include <stream/passthroughsamplerateconverter.h>

namespace xamp::stream {

PassThroughSampleRateConverter::PassThroughSampleRateConverter(DsdModes dsd_mode, uint8_t sample_size)
    : dsd_mode_(dsd_mode)
    , sample_size_(sample_size)
    , dispatch_(nullptr) {
    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        dispatch_ = std::bind(&PassThroughSampleRateConverter::ProcessNativeDsd, this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3);
    }
    else {
        dispatch_ = std::bind(&PassThroughSampleRateConverter::ProcessPcm, this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3);
    }
}

bool PassThroughSampleRateConverter::Process(float const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    return dispatch_(reinterpret_cast<int8_t const*>(sample_buffer), num_samples, buffer);
}

bool PassThroughSampleRateConverter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
    return Process(input.data(), input.size(), buffer);
}

std::string_view PassThroughSampleRateConverter::GetDescription() const noexcept {
    return "Pass Through";
}

bool PassThroughSampleRateConverter::ProcessNativeDsd(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    BufferOverFlowThrow(buffer.TryWrite(sample_buffer, num_samples));
    return true;
}

bool PassThroughSampleRateConverter::ProcessPcm(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    BufferOverFlowThrow(buffer.TryWrite(sample_buffer, num_samples * sample_size_));
    return true;
}

}
