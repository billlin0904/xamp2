//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audioformat.h>

namespace xamp::base {

static const double XAMP_FLOAT_16_SCALER = double(0x7fffL) + .49999;
static const double XAMP_FLOAT_24_SCALER = double(0x7fffffL) + .49999;
static const double XAMP_FLOAT_32_SCALER = double(0x7fffffffL) + .49999;

#pragma pack(push, 1)
class int24_t {
public:
	int24_t() noexcept;

	int24_t(const int24_t& v) noexcept;

	int24_t(float f) noexcept;

	constexpr int24_t& operator=(const int24_t& v) noexcept;

	constexpr int24_t& operator=(const int32_t i) noexcept;

	constexpr int24_t& operator=(const float f) noexcept;

	constexpr int32_t to_2432int() const noexcept;
protected:
	std::array<uint8_t, 3> c3;
};
#pragma pack(pop)

XAMP_ALWAYS_INLINE int24_t::int24_t() noexcept {
	c3.fill(0);
}

XAMP_ALWAYS_INLINE int24_t::int24_t(const int24_t& v) noexcept {
	*this = v;
}

XAMP_ALWAYS_INLINE int24_t::int24_t(float f) noexcept {	
	*this = static_cast<int32_t>(f * XAMP_FLOAT_24_SCALER);
}

XAMP_ALWAYS_INLINE constexpr int24_t& int24_t::operator=(const int24_t& v) noexcept {
	if (this != &v) {
		c3[0] = v.c3[0];
		c3[1] = v.c3[1];
		c3[2] = v.c3[2];
	}
	return *this;
}

XAMP_ALWAYS_INLINE constexpr int24_t& int24_t::operator=(const float f) noexcept {
	*this = static_cast<int32_t>(f * XAMP_FLOAT_24_SCALER);
	return *this;
}

XAMP_ALWAYS_INLINE constexpr int24_t& int24_t::operator=(const int32_t i) noexcept {
	c3[0] = (i & 0x000000ff);
	c3[1] = (i & 0x0000ff00) >> 8;
	c3[2] = (i & 0x00ff0000) >> 16;
	return *this;
}

XAMP_ALWAYS_INLINE constexpr int32_t int24_t::to_2432int() const noexcept {
	return int32_t(c3[0] | (c3[1] << 8) | (c3[2] << 16)) << 8;
}

struct XAMP_BASE_API ConvertContext {
	ConvertContext();
	~ConvertContext() noexcept = default;
	int32_t convert_size{};
	int32_t in_jump{};
	int32_t out_jump{};
	AudioFormat input_format;
	AudioFormat output_format;
	std::array<int32_t, XAMP_MAX_CHANNEL> in_offset;
	std::array<int32_t, XAMP_MAX_CHANNEL> out_offset;
};

XAMP_BASE_API ConvertContext MakeConvert(const AudioFormat& in_format, const AudioFormat& out_format, uint32_t convert_size) noexcept;

template <InterleavedFormat InputFormat, InterleavedFormat OutputFormat>
struct DataConverter {
	static int32_t* Convert(int32_t* output, const float* input, const ConvertContext& context) noexcept {
		const auto output_left_offset = context.out_offset[0];
		const auto output_right_offset = context.out_offset[1];

		const auto input_left_offset = context.in_offset[0];
		const auto input_right_offset = context.in_offset[1];

		for (int32_t i = 0; i < context.convert_size; ++i) {
			output[output_left_offset] = static_cast<int32_t>(input[input_left_offset] * XAMP_FLOAT_32_SCALER);
			output[output_right_offset] = static_cast<int32_t>(input[input_right_offset] * XAMP_FLOAT_32_SCALER);
			input += context.in_jump;
			output += context.out_jump;
		}
		return output;
	}
};

template <>
struct DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED> {
	static int32_t* Convert2432Bit(int32_t* output, const float* input, const ConvertContext& context) noexcept {
		const auto* end_input = input + context.convert_size * context.input_format.GetChannels();

		// Loop unrolling
		switch ((end_input - input) % 8) {
		case 7:
			*output++ = int24_t(*input++).to_2432int();
		case 6:
			*output++ = int24_t(*input++).to_2432int();
		case 5:
			*output++ = int24_t(*input++).to_2432int();
		case 4:
			*output++ = int24_t(*input++).to_2432int();
		case 3:
			*output++ = int24_t(*input++).to_2432int();
		case 2:
			*output++ = int24_t(*input++).to_2432int();
		case 1:
			*output++ = int24_t(*input++).to_2432int();
		case 0:
			break;
		}

		while (input != end_input) {
			output[0] = int24_t(input[0]).to_2432int();
			output[1] = int24_t(input[1]).to_2432int();
			output[2] = int24_t(input[2]).to_2432int();
			output[3] = int24_t(input[3]).to_2432int();
			output[4] = int24_t(input[4]).to_2432int();
			output[5] = int24_t(input[5]).to_2432int();
			output[6] = int24_t(input[6]).to_2432int();
			output[7] = int24_t(input[7]).to_2432int();
			input += 8;
			output += 8;
		}
		return output;
	}
};

}
