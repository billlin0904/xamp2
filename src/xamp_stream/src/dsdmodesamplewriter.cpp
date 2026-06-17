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
        dispatch_ = bind_front(&DsdModeSampleWriter::processNativeDsd, this);
    }
    else {
        dispatch_ = bind_front(&DsdModeSampleWriter::processPcm, this);
    }
}

bool DsdModeSampleWriter::process(float const * sample_buffer, size_t num_samples, AudioBuffer<std::byte>& buffer) {
    return std::invoke(dispatch_, reinterpret_cast<const std::byte*>(sample_buffer), num_samples, buffer);
}

bool DsdModeSampleWriter::process(const BufferRef<float>& input, AudioBuffer<std::byte>& buffer) {
    return process(input.data(), input.size(), buffer);
}

bool DsdModeSampleWriter::processNativeDsd(const std::byte* sample_buffer, size_t num_samples, AudioBuffer<std::byte>& fifo) {
    ThrowIf<BufferOverflowException>(fifo.tryWrite(sample_buffer, num_samples),
        "Failed to write buffer, read:{} write:{}",
        fifo.getAvailableRead(),
        fifo.getAvailableWrite());
    return true;
}

bool DsdModeSampleWriter::processPcm(const std::byte* sample_buffer, size_t num_samples, AudioBuffer<std::byte>& fifo) {
    ThrowIf<BufferOverflowException>(fifo.tryWrite(sample_buffer, num_samples * sample_size_),
        "Failed to write buffer, read:{} write:{}",
        fifo.getAvailableRead(),
        fifo.getAvailableWrite());
    return true;
}

XAMP_STREAM_NAMESPACE_END
