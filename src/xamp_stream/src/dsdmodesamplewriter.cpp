#include <base/exception.h>
#include <base/dataconverter.h>
#include <base/stl.h>
#include <stream/dsdmodesamplewriter.h>

XAMP_STREAM_NAMESPACE_BEGIN

DsdModeSampleWriter::DsdModeSampleWriter(DsdModes dsd_mode, uint8_t sample_size)
    : dsd_mode_(dsd_mode)
    , sample_size_(sample_size)
    , dispatch_(nullptr) {
    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        dispatch_ = bind_front(&DsdModeSampleWriter::ProcessNativeDsd, this);
    }
    else {
        dispatch_ = bind_front(&DsdModeSampleWriter::ProcessPcm, this);
    }
}

bool DsdModeSampleWriter::Process(float const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    return dispatch_(reinterpret_cast<int8_t const*>(sample_buffer), num_samples, buffer);
}

bool DsdModeSampleWriter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
    return Process(input.data(), input.size(), buffer);
}

bool DsdModeSampleWriter::ProcessNativeDsd(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& fifo) {
    ThrowIf<BufferOverflowException>(fifo.TryWrite(sample_buffer, num_samples),
        "Failed to write buffer, read:{} write:{}",
        fifo.GetAvailableRead(),
        fifo.GetAvailableWrite());
    return true;
}

bool DsdModeSampleWriter::ProcessPcm(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& fifo) {
    ThrowIf<BufferOverflowException>(fifo.TryWrite(sample_buffer, num_samples * sample_size_),
        "Failed to write buffer, read:{} write:{}",
        fifo.GetAvailableRead(),
        fifo.GetAvailableWrite());
    return true;
}

XAMP_STREAM_NAMESPACE_END
