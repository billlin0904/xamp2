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
enum class ReadStatus { Samples, End, Underrun };
constexpr ReadStatus classifyRead(size_t bytes, size_t capacity, size_t frame_bytes, bool eof) noexcept {
    if (!frame_bytes || bytes > capacity || bytes % frame_bytes || capacity % frame_bytes)
        return ReadStatus::Underrun;
    if (!bytes) return eof ? ReadStatus::End : ReadStatus::Underrun;
    return bytes == capacity || eof ? ReadStatus::Samples : ReadStatus::Underrun;
}
}
