#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace xamp::pcm {
enum class SampleType { SignedInteger, Float };
enum class ByteOrder { Little, Big };
enum class Alignment { Left, Right };
enum class Layout { Interleaved, Planar };
struct Format {
    uint32_t sample_rate{};
    uint16_t channels{};
    uint16_t valid_bits{};
    uint16_t container_bits{};
    SampleType type{SampleType::SignedInteger};
    ByteOrder byte_order{ByteOrder::Little};
    Alignment alignment{Alignment::Left};
    Layout layout{Layout::Interleaved};
    bool operator==(const Format&) const = default;
    constexpr size_t sampleBytes() const { return container_bits / 8; }
    constexpr size_t frameBytes() const { return sampleBytes() * channels; }
};
struct Block {
    std::span<const std::byte> data;
    size_t frames{};
    Format format;
};
constexpr bool valid(const Format& f) noexcept {
    return f.type == SampleType::SignedInteger && f.sample_rate && f.channels &&
        (f.container_bits == 16 || f.container_bits == 24 || f.container_bits == 32) &&
        f.valid_bits >= 16 && f.valid_bits <= f.container_bits;
}
constexpr bool lossless(const Format& source, const Format& target) noexcept {
    return valid(source) && valid(target) && source.channels == target.channels &&
        source.sample_rate == target.sample_rate && source.valid_bits <= target.valid_bits;
}
constexpr Format canonical(unsigned bits, unsigned channels, unsigned rate) {
    return {rate, static_cast<uint16_t>(channels), static_cast<uint16_t>(bits), 32};
}
inline uint32_t readSignedWord(const std::byte* data, const Format& f) noexcept {
    uint32_t word = 0;
    for (size_t b=0; b<f.sampleBytes(); ++b) {
        const auto shift = 8 * (f.byte_order == ByteOrder::Little ? b : f.sampleBytes()-1-b);
        word |= std::to_integer<uint32_t>(data[b]) << shift;
    }
    if (f.alignment == Alignment::Left) word >>= f.container_bits-f.valid_bits;
    const auto mask = UINT32_MAX >> (32-f.valid_bits);
    word &= mask;
    if (word & (uint32_t{1} << (f.valid_bits-1))) word |= ~mask;
    return word;
}
// Caller supplies one interleaved buffer or one buffer per planar channel.
// Input and output must not overlap, except an exact-format copy using memmove.
// No floating point, quantization, dithering or loss of significant source bits.
inline bool convert(const Block& source, const Format& target,
                    std::span<std::span<std::byte>> outputs) noexcept {
    if (!lossless(source.format,target)) return false;
    if (source.frames > (std::numeric_limits<size_t>::max)()/source.format.frameBytes() ||
        source.frames > (std::numeric_limits<size_t>::max)()/target.frameBytes()) return false;
    const auto input_bytes = source.frames * source.format.frameBytes();
    if (source.data.size() != input_bytes) return false;
    const size_t planes = target.layout == Layout::Planar ? target.channels : 1;
    if (outputs.size() != planes) return false;
    const auto plane_bytes = source.frames * target.frameBytes()/planes;
    for (const auto& out: outputs) if (out.size() < plane_bytes) return false;
    if (source.format == target) {
        for (size_t p=0; p<planes; ++p)
            if (plane_bytes) std::memmove(outputs[p].data(),source.data.data()+p*plane_bytes,plane_bytes);
        return true;
    }
    for (size_t frame=0; frame<source.frames; ++frame) for (size_t ch=0; ch<target.channels; ++ch) {
        const auto sample = source.format.layout == Layout::Interleaved
            ? frame*target.channels+ch : ch*source.frames+frame;
        uint32_t word = readSignedWord(source.data.data()+sample*source.format.sampleBytes(),source.format);
        word <<= target.valid_bits-source.format.valid_bits;
        if (target.alignment == Alignment::Left) word <<= target.container_bits-target.valid_bits;
        auto* dest = target.layout == Layout::Interleaved
            ? outputs[0].data()+(frame*target.channels+ch)*target.sampleBytes()
            : outputs[ch].data()+frame*target.sampleBytes();
        for (size_t b=0; b<target.sampleBytes(); ++b) {
            const auto shift=8*(target.byte_order == ByteOrder::Little ? b : target.sampleBytes()-1-b);
            dest[b]=static_cast<std::byte>((word>>shift)&255);
        }
    }
    return true;
}
}
