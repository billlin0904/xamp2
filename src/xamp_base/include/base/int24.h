//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API int24_t final {
public:
	int24_t() ;

	int24_t(float f) ;

	int24_t& operator=(int32_t i) ;

	int24_t& operator=(float f) ;

	[[nodiscard]] int32_t To2432Int() const ;

	[[nodiscard]] int32_t To32Int() const ;

	std::array<uint8_t, 3> data;
};

XAMP_ALWAYS_INLINE int24_t::int24_t() {
	data.fill(0);
}

XAMP_ALWAYS_INLINE int24_t::int24_t(float f) {
	*this = f;
}

XAMP_ALWAYS_INLINE int24_t& int24_t::operator=(float f) {
	*this = static_cast<int32_t>(f * 8388607.f);
	return *this;
}

XAMP_ALWAYS_INLINE int24_t& int24_t::operator=(int32_t i) {
	data[0] = reinterpret_cast<uint8_t*>(&i)[0];
	data[1] = reinterpret_cast<uint8_t*>(&i)[1];
	data[2] = reinterpret_cast<uint8_t*>(&i)[2];
	//data[0] = reinterpret_cast<uint8_t*>(&i)[1];
	//data[1] = reinterpret_cast<uint8_t*>(&i)[2];
	//data[2] = reinterpret_cast<uint8_t*>(&i)[3];
	return *this;
}

XAMP_ALWAYS_INLINE int32_t int24_t::To2432Int() const {
	return static_cast<int32_t>(static_cast<uint32_t>(To32Int()) << 8);
}

XAMP_ALWAYS_INLINE int32_t int24_t::To32Int() const {
	uint32_t v = static_cast<uint32_t>(data[0])
		| (static_cast<uint32_t>(data[1]) << 8)
		| (static_cast<uint32_t>(data[2]) << 16);
	if ((v & 0x00800000u) != 0) {
		v |= 0xFF000000u;
	}
	return static_cast<int32_t>(v);
}

XAMP_BASE_NAMESPACE_END
