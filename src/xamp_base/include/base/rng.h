//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdlib>

#include <random>
#include <array>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/xoshiro.h>

namespace xamp::base {

class XAMP_BASE_API PRNG final {
public:
    static PRNG& GetInstance() noexcept;

    PRNG() noexcept;

    XAMP_DISABLE_COPY(PRNG)

    template <typename T, std::enable_if_t<std::is_same_v<T, float>>* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_real_distribution<T>(min, max)(engine_);
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution<T>(min, max)(engine_);
    }

    int64_t NextInt64(
        const int64_t min = (std::numeric_limits<int64_t>::min)(),
        const int64_t max = (std::numeric_limits<int64_t>::max)()) noexcept {
        // note: xoshiro隨機數演算法只在64bit資料處理很快, 所以只保留int64_t類型.
        return (*this)(min, max);
    }

    float NextFloat(
        const float min = (std::numeric_limits<float>::min)(),
        const float max = (std::numeric_limits<float>::max)()) noexcept {
        return (*this)(min, max);
    }

	AlignArray<float> GetRandomFloat(size_t size,
		const float min = (std::numeric_limits<float>::min)(),
		const float max = (std::numeric_limits<float>::max)()) {
		auto output = MakeAlignedArray<float>(size);
        for (size_t i = 0; i < size; ++i) {
			output[i] = (*this)(min, max);
		}
		return output;
	}

	AlignArray<int32_t> GetRandomInt(size_t size,
		const int32_t min = (std::numeric_limits<int32_t>::min)(),
		const int32_t max = (std::numeric_limits<int32_t>::max)()) {
		auto output = MakeAlignedArray<int32_t>(size);
        for (size_t i = 0; i < size; ++i) {
			output[i] = (*this)(min, max);
		}
		return output;
	}

	AlignArray<int8_t> GetRandomBytes(size_t size,
		const int32_t min = (std::numeric_limits<int8_t>::min)(),
		const int32_t max = (std::numeric_limits<int8_t>::max)()) {
		auto output = MakeAlignedArray<int8_t>(size);
        for (size_t i = 0; i < size; ++i) {
			output[i] = static_cast<int8_t>((*this)(min, max));
		}
		return output;
	}
private:
    Xoshiro256StarStarEngine engine_;
};

}
