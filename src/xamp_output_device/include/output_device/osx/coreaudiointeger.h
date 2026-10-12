#pragma once

#include <CoreAudio/CoreAudio.h>
#include <base/pcm.h>
#include <optional>
#include <cmath>

namespace xamp::output_device::osx::integer_audio {

// Only one interleaved stereo stream is supported by the strict HAL path.
inline std::optional<xamp::pcm::Format> pcmFormat(const AudioStreamBasicDescription& f) {
    if (f.mFormatID != kAudioFormatLinearPCM ||
        !(f.mFormatFlags & kAudioFormatFlagIsSignedInteger) ||
        (f.mFormatFlags & (kAudioFormatFlagIsFloat | kAudioFormatFlagIsNonInterleaved)) ||
        f.mChannelsPerFrame != 2 || f.mFramesPerPacket != 1 ||
        f.mBytesPerPacket != f.mBytesPerFrame || f.mBytesPerFrame % 2 ||
        f.mBitsPerChannel < 16 || f.mBitsPerChannel > 32 ||
        f.mBytesPerFrame < 4 || f.mBytesPerFrame > 8 ||
        !std::isfinite(f.mSampleRate) || f.mSampleRate <= 0 || f.mSampleRate > UINT32_MAX ||
        f.mSampleRate != static_cast<uint32_t>(f.mSampleRate)) return std::nullopt;
    xamp::pcm::Format result{static_cast<uint32_t>(f.mSampleRate), 2,
        static_cast<uint16_t>(f.mBitsPerChannel), static_cast<uint16_t>(f.mBytesPerFrame * 4)};
    result.byte_order = (f.mFormatFlags & kAudioFormatFlagIsBigEndian)
        ? xamp::pcm::ByteOrder::Big : xamp::pcm::ByteOrder::Little;
    result.alignment = (f.mFormatFlags & kAudioFormatFlagIsAlignedHigh)
        ? xamp::pcm::Alignment::Left : xamp::pcm::Alignment::Right;
    if (!xamp::pcm::valid(result) || f.mBitsPerChannel != result.valid_bits ||
        f.mBytesPerFrame != result.frameBytes()) return std::nullopt;
    return result;
}

inline bool sameFormat(const AudioStreamBasicDescription& a, const AudioStreamBasicDescription& b) {
    return a.mSampleRate == b.mSampleRate && a.mFormatID == b.mFormatID &&
        a.mFormatFlags == b.mFormatFlags && a.mBytesPerPacket == b.mBytesPerPacket &&
        a.mFramesPerPacket == b.mFramesPerPacket && a.mBytesPerFrame == b.mBytesPerFrame &&
        a.mChannelsPerFrame == b.mChannelsPerFrame && a.mBitsPerChannel == b.mBitsPerChannel;
}

inline std::optional<AudioStreamBasicDescription> selectFormat(
    std::span<const AudioStreamRangedDescription> physical,
    std::span<const AudioStreamRangedDescription> virtual_formats,
    const xamp::pcm::Format& source) {
    std::optional<AudioStreamBasicDescription> selected;
    for (auto candidate : physical) {
        if (source.sample_rate < candidate.mSampleRateRange.mMinimum ||
            source.sample_rate > candidate.mSampleRateRange.mMaximum) continue;
        candidate.mFormat.mSampleRate = source.sample_rate;
        const auto pcm = pcmFormat(candidate.mFormat);
        if (!pcm || !xamp::pcm::lossless(source, *pcm)) continue;
        for (auto virtual_format : virtual_formats) {
            if (source.sample_rate < virtual_format.mSampleRateRange.mMinimum ||
                source.sample_rate > virtual_format.mSampleRateRange.mMaximum) continue;
            virtual_format.mFormat.mSampleRate = source.sample_rate;
            if (!sameFormat(candidate.mFormat, virtual_format.mFormat)) continue;
            if (!selected || candidate.mFormat.mBitsPerChannel < selected->mBitsPerChannel)
                selected = candidate.mFormat;
        }
    }
    return selected;
}

// The source is canonical, little-endian, left-aligned 24-in-32 DoP.
// Regenerate framing across both audio and idle frames, including odd callback sizes.
inline void frameDop(std::span<std::byte> buffer, size_t audio_frames, uint8_t& marker) {
    const size_t frames = buffer.size() / 8;
    for (size_t frame = 0; frame < frames; ++frame) {
        for (size_t channel = 0; channel < 2; ++channel) {
            auto* sample = buffer.data() + frame * 8 + channel * 4;
            sample[0] = std::byte{0};
            if (frame >= audio_frames) sample[1] = sample[2] = std::byte{0x69};
            sample[3] = static_cast<std::byte>(marker);
        }
        marker = marker == 0x05 ? 0xfa : 0x05;
    }
}
}
