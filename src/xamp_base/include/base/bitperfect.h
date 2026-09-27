#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace xamp::bitperfect {
// Lossless stereo integer PCM source policy; outputs negotiate their own containers.
constexpr bool supported(unsigned bits, unsigned channels, unsigned rate) noexcept {
    return (bits == 16 || bits == 24 || bits == 32) && channels == 2 && rate != 0;
}
// Producer scheduling is based on one complete decoded block, not a multiple
// of the device callback size (which may exceed FIFO capacity).
constexpr bool canRefill(size_t available_write, size_t block_bytes, size_t minimum_bytes) noexcept {
    return available_write >= (block_bytes > minimum_bytes ? block_bytes : minimum_bytes);
}
enum class ReadStatus {
    Samples, End, InvalidFrameSize, ExceedsCapacity,
    MisalignedRead, MisalignedCapacity, EmptyBuffer, ShortRead
};
constexpr ReadStatus classifyRead(size_t bytes, size_t capacity, size_t frame_bytes, bool eof) noexcept {
    if (!frame_bytes) return ReadStatus::InvalidFrameSize;
    if (bytes > capacity) return ReadStatus::ExceedsCapacity;
    if (bytes % frame_bytes) return ReadStatus::MisalignedRead;
    if (capacity % frame_bytes) return ReadStatus::MisalignedCapacity;
    if (!bytes) return eof ? ReadStatus::End : ReadStatus::EmptyBuffer;
    if (bytes == capacity || eof) return ReadStatus::Samples;
    return ReadStatus::ShortRead;
}
constexpr const char* readStatusName(ReadStatus status) noexcept {
    switch (status) {
    case ReadStatus::Samples: return "Samples";
    case ReadStatus::End: return "End";
    case ReadStatus::InvalidFrameSize: return "InvalidFrameSize";
    case ReadStatus::ExceedsCapacity: return "ExceedsCapacity";
    case ReadStatus::MisalignedRead: return "MisalignedRead";
    case ReadStatus::MisalignedCapacity: return "MisalignedCapacity";
    case ReadStatus::EmptyBuffer: return "EmptyBuffer (audio buffer underrun)";
    case ReadStatus::ShortRead: return "ShortRead (audio buffer underrun)";
    }
    return "Unknown";
}
}
