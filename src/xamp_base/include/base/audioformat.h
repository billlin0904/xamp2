//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ostream>
#include <functional>
#include <iomanip>

#include <base/base.h>
#include <base/enum.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* Byte format
* 
* <remarks>
* INVALID_FORMAT: invalid format
* SINT8: 8 bit signed integer
* SINT16: 16 bit signed integer
* SINT24: 24 bit signed integer
* SINT32: 32 bit signed integer
* FLOAT32: 32 bit float
* FLOAT64: 64 bit float
* </remarks>
*/
XAMP_MAKE_ENUM(ByteFormat,
          INVALID_FORMAT,
		  SINT8,
          SINT16,
          SINT24,
          SINT32,
          FLOAT32,
          FLOAT64)

   
/*
* Packed format
* 
* <remarks>
* INTERLEAVED: interleaved
* PLANAR: planar
* </remarks>
*/
XAMP_MAKE_ENUM(PackedFormat,
          INTERLEAVED,
          PLANAR)

/*
* Data format
* 
* <remarks>
* FORMAT_DSD: DSD
* FORMAT_PCM: PCM
* </remarks>
*/
XAMP_MAKE_ENUM(DataFormat,
          FORMAT_DSD,
          FORMAT_PCM)

#define DECLARE_AUDIO_FORMAT(Name) \
    static const AudioFormat k16Bit##Name;\
    static const AudioFormat k24Bit##Name;\
	static const AudioFormat kFloat##Name

/*
* Audio format
* 
* <remarks>
* Audio format is a combination of data format, sample rate, bits per sample, number of channels and byte format.
* </remarks>
*/
class XAMP_BASE_API AudioFormat final {
public:
    static const AudioFormat kUnknownFormat;
    static constexpr uint32_t kMaxChannel = 2;

    DECLARE_AUDIO_FORMAT(PCM441Khz);
    DECLARE_AUDIO_FORMAT(PCM48Khz);
    DECLARE_AUDIO_FORMAT(PCM96Khz);
    DECLARE_AUDIO_FORMAT(PCM882Khz);
    DECLARE_AUDIO_FORMAT(PCM1764Khz);
    DECLARE_AUDIO_FORMAT(PCM192Khz);
    DECLARE_AUDIO_FORMAT(PCM3528Khz);
    DECLARE_AUDIO_FORMAT(PCM384Khz);
    DECLARE_AUDIO_FORMAT(PCM768Khz);

    explicit AudioFormat(DataFormat format = DataFormat::FORMAT_PCM,
                         uint16_t number_of_channels = 0,
                         uint32_t bits_per_sample = 0,
                         uint32_t sample_rate = 0) noexcept;

    explicit AudioFormat(DataFormat format,
                         uint16_t number_of_channels,
                         ByteFormat byte_format,
                         uint32_t sample_rate,
                         PackedFormat packed_format = PackedFormat::INTERLEAVED) noexcept;

    void SetFormat(DataFormat format) noexcept;

    void SetSampleRate(uint32_t sample_rate) noexcept;

    void SetBitPerSample(uint32_t bits_per_sample) noexcept;

    void SetChannel(uint16_t num_channels) noexcept;

    void SetByteFormat(ByteFormat format) noexcept;

    void SetPackedFormat(PackedFormat format) noexcept;

    [[nodiscard]] DataFormat GetFormat() const noexcept;

    [[nodiscard]] PackedFormat GetPackedFormat() const noexcept;

    [[nodiscard]] uint32_t GetSampleRate() const noexcept;

    [[nodiscard]] uint32_t GetAvgBytesPerSec() const noexcept;

    [[nodiscard]] uint32_t GetAvgFramesPerSec() const noexcept;

    [[nodiscard]] uint16_t GetChannels() const noexcept;

    [[nodiscard]] uint32_t GetBitsPerSample() const noexcept;

    [[nodiscard]] uint32_t GetBytesPerSample() const noexcept;

    [[nodiscard]] uint32_t GetSampleSize() const noexcept;

    [[nodiscard]] uint32_t GetBlockAlign() const noexcept;

    [[nodiscard]] ByteFormat GetByteFormat() const noexcept;

    [[nodiscard]] uint64_t GetSecondsSize(double sec) const noexcept;

    void Reset() noexcept;

    static AudioFormat ToFloatFormat(AudioFormat const& source_format) noexcept;

    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] std::string ToShortString() const;

    [[nodiscard]] size_t GetHash() const;

private:
    XAMP_BASE_API friend bool operator>(const AudioFormat& format, const AudioFormat& other) noexcept;

    XAMP_BASE_API friend bool operator==(const AudioFormat& format, const AudioFormat& other) noexcept;

    XAMP_BASE_API friend bool operator!=(const AudioFormat& format, const AudioFormat& other) noexcept;

    XAMP_BASE_API friend std::ostream& operator<<(std::ostream& ostr, const AudioFormat& format);

    DataFormat format_;
    ByteFormat byte_format_;
    PackedFormat packed_format_;
    uint32_t num_channels_;
    uint32_t sample_rate_;
    uint32_t bits_per_sample_;
};

XAMP_ALWAYS_INLINE AudioFormat::AudioFormat(DataFormat format,
    uint16_t number_of_channels,
    ByteFormat byte_format,
    uint32_t sample_rate,
    PackedFormat interleaved_format) noexcept
    : format_(format)
    , byte_format_(ByteFormat::INVALID_FORMAT)
    , packed_format_(interleaved_format)
    , num_channels_(number_of_channels)
    , sample_rate_(sample_rate) {
    SetByteFormat(byte_format);
}

XAMP_ALWAYS_INLINE AudioFormat::AudioFormat(DataFormat format,
    uint16_t number_of_channels,
    uint32_t bits_per_sample,
    uint32_t sample_rate) noexcept
    : format_(format)
    , byte_format_(ByteFormat::INVALID_FORMAT)
    , packed_format_(PackedFormat::INTERLEAVED)
    , num_channels_(number_of_channels)
    , sample_rate_(sample_rate) {
    SetBitPerSample(bits_per_sample);
}

XAMP_ALWAYS_INLINE DataFormat AudioFormat::GetFormat() const noexcept {
    return format_;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetFormat(DataFormat format) noexcept {
    format_ = format;
}

XAMP_ALWAYS_INLINE uint16_t AudioFormat::GetChannels() const noexcept {
    return num_channels_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetBitsPerSample() const noexcept {
    return bits_per_sample_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetBytesPerSample() const noexcept {
    return bits_per_sample_ / 8;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetSampleSize() const noexcept {
    return GetBytesPerSample() * GetChannels();
}

XAMP_ALWAYS_INLINE void AudioFormat::SetSampleRate(uint32_t sample_rate) noexcept {
    sample_rate_ = sample_rate;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetChannel(uint16_t num_channels) noexcept {
    num_channels_ = num_channels;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetBitPerSample(uint32_t bits_per_sample) noexcept {
    switch (bits_per_sample) {
    case 8:
        SetByteFormat(ByteFormat::SINT8);
        break;
    case 16:
        SetByteFormat(ByteFormat::SINT16);
        break;
    case 24:
        SetByteFormat(ByteFormat::SINT24);
        break;
    case 32:
        SetByteFormat(ByteFormat::SINT32);
        break;
    default:
        SetByteFormat(ByteFormat::INVALID_FORMAT);
        break;
    }
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetSampleRate() const noexcept {
    return sample_rate_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetAvgBytesPerSec() const noexcept {
    return GetSampleRate() * GetBlockAlign();
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetAvgFramesPerSec() const noexcept {
    return GetSampleRate() * GetChannels();
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetBlockAlign() const noexcept {
    return GetBytesPerSample() * GetChannels();
}

XAMP_ALWAYS_INLINE ByteFormat AudioFormat::GetByteFormat() const noexcept {
    return byte_format_;
}

XAMP_ALWAYS_INLINE uint64_t AudioFormat::GetSecondsSize(double sec) const noexcept {
    return static_cast<uint64_t>(GetSampleRate() * GetBytesPerSample() * GetChannels() * sec);
}

XAMP_ALWAYS_INLINE void AudioFormat::SetByteFormat(ByteFormat format) noexcept {
    switch (format) {
    case ByteFormat::FLOAT64:
        bits_per_sample_ = 64;
        byte_format_ = format;
        break;
    case ByteFormat::SINT32:
    case ByteFormat::FLOAT32:
        bits_per_sample_ = 32;
        byte_format_ = format;
        break;
    case ByteFormat::SINT24:
        bits_per_sample_ = 24;
        byte_format_ = format;
        break;
    case ByteFormat::SINT16:
        bits_per_sample_ = 16;
        byte_format_ = format;
        break;
    case ByteFormat::SINT8:
        bits_per_sample_ = 8;
        byte_format_ = format;
        break;
    case ByteFormat::INVALID_FORMAT:
        bits_per_sample_ = 0;
        byte_format_ = format;
        break;
    }
}

XAMP_ALWAYS_INLINE void AudioFormat::SetPackedFormat(PackedFormat format) noexcept {
    packed_format_ = format;
}

XAMP_ALWAYS_INLINE PackedFormat AudioFormat::GetPackedFormat() const noexcept {
    return packed_format_;
}

XAMP_ALWAYS_INLINE std::ostream& operator<<(std::ostream& ostr, AudioFormat const & format) {
    ostr << format.GetByteFormat() << "-" << format.GetPackedFormat() << "-"
         << format.ToShortString();
    return ostr;
}

XAMP_ALWAYS_INLINE bool operator>(const AudioFormat& format, const AudioFormat& other) noexcept {
    return format.GetBitsPerSample() > other.GetBitsPerSample()
        && format.GetSampleRate() > other.GetSampleRate();
}

XAMP_ALWAYS_INLINE bool operator!=(AudioFormat const & format, AudioFormat const & other) noexcept {
    return format.GetHash() != other.GetHash();
}

XAMP_ALWAYS_INLINE bool operator==(AudioFormat const & format, AudioFormat const & other) noexcept {
    return format.GetHash() == other.GetHash();
}

XAMP_ALWAYS_INLINE void AudioFormat::Reset() noexcept {
    *this = kUnknownFormat;
}

XAMP_BASE_NAMESPACE_END

template <>
struct std::hash<xamp::base::AudioFormat> {
    std::size_t operator()(xamp::base::AudioFormat const& f) const noexcept {
        return f.GetHash();
    }
};