//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/base.h>
#include <base/simd.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API Int24 final {
public:
	Int24() noexcept;

	Int24(float f) noexcept;

	Int24& operator=(int32_t i) noexcept;

	Int24& operator=(float f) noexcept;

	XAMP_NO_DISCARD int32_t To2432Int() const noexcept;

	XAMP_NO_DISCARD int32_t To32Int() const noexcept;

	std::array<uint8_t, 3> data;
};

XAMP_ALWAYS_INLINE Int24::Int24() noexcept {
	data.fill(0);
}

XAMP_ALWAYS_INLINE Int24::Int24(float f) noexcept {
	*this = f;
}

XAMP_ALWAYS_INLINE Int24& Int24::operator=(float f) noexcept {
	*this = static_cast<int32_t>(f * kFloat24Scale);
	return *this;
}

XAMP_ALWAYS_INLINE Int24& Int24::operator=(int32_t i) noexcept {
	data[0] = reinterpret_cast<uint8_t*>(&i)[0];
	data[1] = reinterpret_cast<uint8_t*>(&i)[1];
	data[2] = reinterpret_cast<uint8_t*>(&i)[2];
	return *this;
}

XAMP_ALWAYS_INLINE int32_t Int24::To2432Int() const noexcept {
	int32_t v{ 0 };
	MemoryCopy(&v, &data[0], 3);
	return v << 8;
}

XAMP_ALWAYS_INLINE int32_t Int24::To32Int() const noexcept {
	int32_t v{ 0 };
	MemoryCopy(&v, &data[0], 3);
	return v;
}

XAMP_BASE_NAMESPACE_END
