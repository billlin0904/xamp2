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

bool DsdModeSampleWriter::Process(float const * sample_buffer, size_t num_samples, AudioBuffer<std::byte>& buffer) {
    return std::invoke(dispatch_, reinterpret_cast<const std::byte*>(sample_buffer), num_samples, buffer);
}

bool DsdModeSampleWriter::Process(const BufferRef<float>& input, AudioBuffer<std::byte>& buffer) {
    return Process(input.data(), input.size(), buffer);
}

Uuid DsdModeSampleWriter::GetTypeId() const {
    return XAMP_UUID_OF(DsdModeSampleWriter);
}

bool DsdModeSampleWriter::ProcessNativeDsd(const std::byte* sample_buffer, size_t num_samples, AudioBuffer<std::byte>& fifo) {
    ThrowIf<BufferOverflowException>(fifo.TryWrite(sample_buffer, num_samples),
        "Failed to write buffer, read:{} write:{}",
        fifo.GetAvailableRead(),
        fifo.GetAvailableWrite());
    return true;
}

bool DsdModeSampleWriter::ProcessPcm(const std::byte* sample_buffer, size_t num_samples, AudioBuffer<std::byte>& fifo) {
    ThrowIf<BufferOverflowException>(fifo.TryWrite(sample_buffer, num_samples * sample_size_),
        "Failed to write buffer, read:{} write:{}",
        fifo.GetAvailableRead(),
        fifo.GetAvailableWrite());
    return true;
}

XAMP_STREAM_NAMESPACE_END
