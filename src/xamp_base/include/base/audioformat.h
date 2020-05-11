//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ostream>
#include <sstream>
#include <iomanip>

#include <base/base.h>
#include <base/enum.h>

namespace xamp::base {

MAKE_ENUM(ByteFormat,
	INVALID_FORMAT,
	SINT16,
	SINT24,
	// Not support formats.
	SINT8,
	SINT32,
	FLOAT32,
	FLOAT64)

MAKE_ENUM(InterleavedFormat,
	// ¥æ´À
	INTERLEAVED,
	// ¥­­±
	DEINTERLEAVED
)

MAKE_ENUM(DataFormat,
	FORMAT_DSD,
	FORMAT_PCM,
	)

constexpr uint32_t kMaxChannel = 2;
constexpr uint32_t kMaxSamplerate = 192000;

class XAMP_BASE_API AudioFormat final {
public:
	static const AudioFormat UnknowFormat;

	explicit AudioFormat(DataFormat format = DataFormat::FORMAT_PCM,
		uint32_t number_of_channels = 0,
		uint32_t bits_per_sample = 0,
		uint32_t samplerate = 0) noexcept;

	explicit AudioFormat(DataFormat format,
		uint32_t number_of_channels,
		ByteFormat byte_format,
		uint32_t samplerate,
		InterleavedFormat interleaved_format = InterleavedFormat::INTERLEAVED) noexcept;

	[[nodiscard]] DataFormat GetFormat() const noexcept;

	void SetFormat(DataFormat format) noexcept;

	void SetSampleRate(uint32_t sample_rate) noexcept;

	void SetBitPerSample(uint32_t bits_per_sample) noexcept;

	void SetChannel(uint32_t num_channels) noexcept;

	[[nodiscard]] uint32_t GetSampleRate() const noexcept;

	[[nodiscard]] uint32_t GetAvgBytesPerSec() const noexcept;

	[[nodiscard]] uint32_t GetAvgFramesPerSec() const noexcept;

	[[nodiscard]] uint32_t GetChannels() const noexcept;

	[[nodiscard]] uint32_t GetBitsPerSample() const noexcept;

	[[nodiscard]] uint32_t GetBytesPerSample() const noexcept;

	[[nodiscard]] uint32_t GetBlockAlign() const noexcept;

	[[nodiscard]] ByteFormat GetByteFormat() const noexcept;

	void SetByteFormat(ByteFormat format) noexcept;

	void SetInterleavedFormat(InterleavedFormat format) noexcept;

	[[nodiscard]] InterleavedFormat GetInterleavedFormat() const noexcept;

	void Reset() noexcept;

private:
	friend bool operator==(const AudioFormat& format, const AudioFormat& other) noexcept;

	friend bool operator!=(const AudioFormat& format, const AudioFormat& other) noexcept;

	friend std::ostream& operator<<(std::ostream& ostr, const AudioFormat& format);

	DataFormat format_;
	ByteFormat byte_format_;
	InterleavedFormat interleaved_format_;
	uint32_t num_channels_;
	uint32_t sample_rate_;
	uint32_t bits_per_sample_;
};

XAMP_ENFORCE_TRIVIAL(AudioFormat);

XAMP_ALWAYS_INLINE AudioFormat::AudioFormat(DataFormat format,
	uint32_t number_of_channels,
	ByteFormat byte_format,
	uint32_t samplerate,
	InterleavedFormat interleaved_format) noexcept
	: format_(format)
	, byte_format_(ByteFormat::INVALID_FORMAT)
	, interleaved_format_(interleaved_format)
	, num_channels_(number_of_channels)
	, sample_rate_(samplerate) {
	SetByteFormat(byte_format);
}

XAMP_ALWAYS_INLINE AudioFormat::AudioFormat(DataFormat format,
	uint32_t number_of_channels,
	uint32_t bits_per_sample,
	uint32_t samplerate) noexcept
	: format_(format)
	, byte_format_(ByteFormat::INVALID_FORMAT)
	, interleaved_format_(InterleavedFormat::INTERLEAVED)
	, num_channels_(number_of_channels)
	, sample_rate_(samplerate) {
	SetBitPerSample(bits_per_sample);
}

XAMP_ALWAYS_INLINE DataFormat AudioFormat::GetFormat() const noexcept {
	return format_;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetFormat(DataFormat format) noexcept {
	format_ = format;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetChannels() const noexcept {
	return num_channels_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetBitsPerSample() const noexcept {
	return bits_per_sample_;
}

XAMP_ALWAYS_INLINE uint32_t AudioFormat::GetBytesPerSample() const noexcept {
	return bits_per_sample_ / 8;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetSampleRate(const uint32_t sample_rate) noexcept {
	sample_rate_ = sample_rate;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetChannel(uint32_t num_channels) noexcept {
	num_channels_ = num_channels;
}

XAMP_ALWAYS_INLINE void AudioFormat::SetBitPerSample(const uint32_t bits_per_sample) noexcept {
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

XAMP_ALWAYS_INLINE void AudioFormat::SetInterleavedFormat(InterleavedFormat format) noexcept {
	interleaved_format_ = format;
}

XAMP_ALWAYS_INLINE InterleavedFormat AudioFormat::GetInterleavedFormat() const noexcept {
	return interleaved_format_;
}

XAMP_ALWAYS_INLINE std::ostream& operator<<(std::ostream& ostr, const AudioFormat& format) {
	ostr << format.GetByteFormat() << "/" << format.GetInterleavedFormat() << "/"
		<< format.GetBitsPerSample() << "bits/";

	if (format.GetSampleRate() % 1000 > 0) {
		ostr << std::fixed << std::setprecision(1) << static_cast<float>(format.GetSampleRate()) / 1000.0f << " Khz";
	}
	else {
		ostr << format.GetSampleRate() / 1000 << "kHZ";
	}

	return ostr;
}

XAMP_ALWAYS_INLINE bool operator!=(const AudioFormat& format, const AudioFormat& other) noexcept {
	return format.format_ != other.format_
		|| format.num_channels_ != other.num_channels_
		|| format.sample_rate_ != other.sample_rate_
		|| format.bits_per_sample_ != other.bits_per_sample_
		|| format.byte_format_ != other.byte_format_
		|| format.interleaved_format_ != other.interleaved_format_;
}

XAMP_ALWAYS_INLINE bool operator==(const AudioFormat& format, const AudioFormat& other) noexcept {
	return format.format_ == other.format_
		&& format.num_channels_ == other.num_channels_
		&& format.sample_rate_ == other.sample_rate_
		&& format.bits_per_sample_ == other.sample_rate_
		&& format.byte_format_ == other.byte_format_
		&& format.interleaved_format_ == other.interleaved_format_;
}

XAMP_ALWAYS_INLINE void AudioFormat::Reset() noexcept {
	*this = UnknowFormat;
}

}

