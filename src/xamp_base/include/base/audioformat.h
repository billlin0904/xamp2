//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ostream>
#include <functional>
#include <iomanip>

#include <base/base.h>
#include <base/enum.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(ByteFormat,
          INVALID_FORMAT,
		  SINT8,
          SINT16,
          SINT24,
          SINT32,
          FLOAT32,
          FLOAT64)

XAMP_MAKE_ENUM(PackedFormat,
          INTERLEAVED,
          PLANAR)

XAMP_MAKE_ENUM(DataFormat,
          FORMAT_DSD,
          FORMAT_PCM)

#define DECLARE_AUDIO_FORMAT(Name) \
    static const AudioFormat k16Bit##Name;\
    static const AudioFormat k24Bit##Name;\
	static const AudioFormat kFloat##Name

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
                         uint32_t sample_rate = 0) ;

    explicit AudioFormat(DataFormat format,
                         uint16_t number_of_channels,
                         ByteFormat byte_format,
                         uint32_t sample_rate,
                         PackedFormat packed_format = PackedFormat::INTERLEAVED) ;

    void setFormat(DataFormat format) ;

    void setSampleRate(uint32_t sample_rate) ;

    void setBitPerSample(uint32_t bits_per_sample) ;

    void setChannel(uint16_t num_channels) ;

    void setByteFormat(ByteFormat format) ;

    void setPackedFormat(PackedFormat format) ;

    [[nodiscard]] DataFormat getFormat() const ;

    [[nodiscard]] PackedFormat getPackedFormat() const ;

    [[nodiscard]] uint32_t getSampleRate() const ;

    [[nodiscard]] uint32_t getAvgBytesPerSec() const ;

    [[nodiscard]] uint32_t getAvgFramesPerSec() const ;

    [[nodiscard]] uint16_t getChannels() const ;

    [[nodiscard]] uint32_t getBitsPerSample() const ;

    [[nodiscard]] uint32_t getBytesPerSample() const ;

    [[nodiscard]] uint32_t getSampleSize() const ;

    [[nodiscard]] uint32_t getBlockAlign() const ;

    [[nodiscard]] ByteFormat getByteFormat() const ;

    [[nodiscard]] uint64_t getSecondsSize(double sec) const ;

    void reset() ;

    static AudioFormat toFloatFormat(AudioFormat const& source_format) ;

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::string toShortString() const;

    [[nodiscard]] size_t getHash() const;

private:
    XAMP_BASE_API friend bool operator>(const AudioFormat& format, const AudioFormat& other) ;

    XAMP_BASE_API friend bool operator==(const AudioFormat& format, const AudioFormat& other) ;

    XAMP_BASE_API friend bool operator!=(const AudioFormat& format, const AudioFormat& other) ;

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
    PackedFormat interleaved_format) : format_(format)
    , byte_format_(ByteFormat::INVALID_FORMAT)
    , packed_format_(interleaved_format)
    , num_channels_(number_of_channels)
    , sample_rate_(sample_rate) {
    setByteFormat(byte_format);
}

XAMP_ALWAYS_INLINE AudioFormat::AudioFormat(DataFormat format,
    uint16_t number_of_channels,
    uint32_t bits_per_sample,
    uint32_t sample_rate) : format_(format)
    , byte_format_(ByteFormat::INVALID_FORMAT)
    , packed_format_(PackedFormat::INTERLEAVED)
    , num_channels_(number_of_channels)
    , sample_rate_(sample_rate) {
    setBitPerSample(bits_per_sample);
}

XAMP_ALWAYS_INLINE DataFormat AudioFormat::getFormat() const {
    return format_;
}

XAMP_ALWAYS_INLINE void AudioFormat::setFormat(DataFormat format) {
    format_ = format;
}

XAMP_ALWAYS_INLINE uint16_t AudioFormat::getChannels() const {
    return num_channels_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getBitsPerSample() const {
    return bits_per_sample_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getBytesPerSample() const {
    return bits_per_sample_ / 8;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getSampleSize() const {
    return getBytesPerSample() * getChannels();
}

XAMP_ALWAYS_INLINE void AudioFormat::setSampleRate(uint32_t sample_rate) {
    sample_rate_ = sample_rate;
}

XAMP_ALWAYS_INLINE void AudioFormat::setChannel(uint16_t num_channels) {
    num_channels_ = num_channels;
}

XAMP_ALWAYS_INLINE void AudioFormat::setBitPerSample(uint32_t bits_per_sample) {
    switch (bits_per_sample) {
    case 8:
        setByteFormat(ByteFormat::SINT8);
        break;
    case 16:
        setByteFormat(ByteFormat::SINT16);
        break;
    case 24:
        setByteFormat(ByteFormat::SINT24);
        break;
    case 32:
        setByteFormat(ByteFormat::SINT32);
        break;
    default:
        setByteFormat(ByteFormat::INVALID_FORMAT);
        break;
    }
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getSampleRate() const {
    return sample_rate_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getAvgBytesPerSec() const {
    return getSampleRate() * getBlockAlign();
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getAvgFramesPerSec() const {
    return getSampleRate() * getChannels();
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::getBlockAlign() const {
    return getBytesPerSample() * getChannels();
}

XAMP_ALWAYS_INLINE ByteFormat AudioFormat::getByteFormat() const {
    return byte_format_;
}

XAMP_ALWAYS_INLINE uint64_t AudioFormat::getSecondsSize(double sec) const {
    return static_cast<uint64_t>(getSampleRate() * getBytesPerSample() * getChannels() * sec);
}

XAMP_ALWAYS_INLINE void AudioFormat::setByteFormat(ByteFormat format) {
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
    default:
        bits_per_sample_ = 0;
        byte_format_ = format;
        break;
    }
}

XAMP_ALWAYS_INLINE void AudioFormat::setPackedFormat(PackedFormat format) {
    packed_format_ = format;
}

XAMP_ALWAYS_INLINE PackedFormat AudioFormat::getPackedFormat() const {
    return packed_format_;
}

XAMP_ALWAYS_INLINE std::ostream& operator<<(std::ostream& ostr, AudioFormat const & format) {
    ostr << format.getByteFormat() << "-" << format.getPackedFormat() << "-"
         << format.toShortString();
    return ostr;
}

XAMP_ALWAYS_INLINE bool operator>(const AudioFormat& format, const AudioFormat& other) {
    return format.getBitsPerSample() > other.getBitsPerSample()
        && format.getSampleRate() > other.getSampleRate();
}

XAMP_ALWAYS_INLINE bool operator!=(AudioFormat const & format, AudioFormat const & other) {
    return format.getHash() != other.getHash();
}

XAMP_ALWAYS_INLINE bool operator==(AudioFormat const & format, AudioFormat const & other) {
    return format.getHash() == other.getHash();
}

XAMP_ALWAYS_INLINE void AudioFormat::reset() {
    *this = kUnknownFormat;
}

XAMP_BASE_NAMESPACE_END

template <>
struct std::hash<xamp::base::AudioFormat> {
    size_t operator()(xamp::base::AudioFormat const& f) const {
        return f.getHash();
    }
};
