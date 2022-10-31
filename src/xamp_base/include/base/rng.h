//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdlib>

#include <random>
#include <array>

#include <base/base.h>
#include <base/stl.h>
#include <base/xoshiro.h>

namespace xamp::base {

class XAMP_BASE_API PRNG final {
public:
    PRNG() noexcept;

    template <typename T, std::enable_if_t<std::is_same_v<T, float>>* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_real_distribution<T>(min, max)(engine_);
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution<T>(min, max)(engine_);
    }

    size_t Next(size_t max) noexcept {
        return (*this)(size_t{0}, max);
    }

    int32_t NextInt32(
        const int32_t min = (std::numeric_limits<int32_t>::min)(),
        const int32_t max = (std::numeric_limits<int32_t>::max)())  noexcept {
        return (*this)(min, max);
    }

    int64_t NextInt64(
        const int64_t min = (std::numeric_limits<int64_t>::min)(),
        const int64_t max = (std::numeric_limits<int64_t>::max)())  noexcept {
        return (*this)(min, max);
    }

    float NextSingle(
        const float min = (std::numeric_limits<float>::min)(),
        const float max = (std::numeric_limits<float>::max)()) noexcept {
        return (*this)(min, max);
    }

    Vector<int8_t> NextBytes(size_t size,
        const int32_t min = (std::numeric_limits<int8_t>::min)(),
        const int32_t max = (std::numeric_limits<int8_t>::max)()) {
        Vector<int8_t> output(size);
        for (size_t i = 0; i < size; ++i) {
            output[i] = static_cast<int8_t>((*this)(min, max));
        }
        return output;
    }

    template <typename T>
	Vector<T> NextBytes(size_t size,
		const T min = (std::numeric_limits<T>::min)(),
		const T max = (std::numeric_limits<T>::max)()) {
        Vector<T> output(size);
        for (size_t i = 0; i < size; ++i) {
			output[i] = static_cast<T>((*this)(min, max));
		}
		return output;
	}

    void SetSeed();
private:
    Xoshiro256StarStarEngine engine_;
};

}
