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
	int24_t() noexcept;

	int24_t(float f) noexcept;

	int24_t& operator=(int32_t i) noexcept;

	int24_t& operator=(float f) noexcept;

	[[nodiscard]] int32_t To2432Int() const noexcept;

	[[nodiscard]] int32_t To32Int() const noexcept;

	std::array<uint8_t, 3> data;
};

XAMP_ALWAYS_INLINE int24_t::int24_t() noexcept {
	data.fill(0);
}

XAMP_ALWAYS_INLINE int24_t::int24_t(float f) noexcept {
	*this = f;
}

XAMP_ALWAYS_INLINE int24_t& int24_t::operator=(float f) noexcept {
	*this = static_cast<int32_t>(f * kFloat24Scale);
	return *this;
}

XAMP_ALWAYS_INLINE int24_t& int24_t::operator=(int32_t i) noexcept {
	data[0] = reinterpret_cast<uint8_t*>(&i)[0];
	data[1] = reinterpret_cast<uint8_t*>(&i)[1];
	data[2] = reinterpret_cast<uint8_t*>(&i)[2];
	//data[0] = reinterpret_cast<uint8_t*>(&i)[1];
	//data[1] = reinterpret_cast<uint8_t*>(&i)[2];
	//data[2] = reinterpret_cast<uint8_t*>(&i)[3];
	return *this;
}

XAMP_ALWAYS_INLINE int32_t int24_t::To2432Int() const noexcept {
	int32_t v{ 0 };
	MemoryCopy(&v, &data[0], 3);
	return v << 8;
}

XAMP_ALWAYS_INLINE int32_t int24_t::To32Int() const noexcept {
	int32_t v{ 0 };
	MemoryCopy(&v, &data[0], 3);
	return v;
}

XAMP_BASE_NAMESPACE_END
