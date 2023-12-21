//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdlib>

#include <random>
#include <array>

#include <base/base.h>
#include <base/stl.h>
#include <base/sfc64.h>

XAMP_BASE_NAMESPACE_BEGIN

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

    size_t Next(size_t max = (std::numeric_limits<size_t>::max)()) noexcept {
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
        const auto gen = [this, min, max]() noexcept {
            return static_cast<int8_t>(NextInt32(min, max));
        };
        std::generate_n(output.begin(), size, gen);
        return output;
    }

    template <typename T>
	Vector<T> NextBytes(size_t size,
		const T min = (std::numeric_limits<T>::min)(),
		const T max = (std::numeric_limits<T>::max)()) {        
        Vector<T> output(size);
        const auto gen = [this, min, max]() noexcept {
            return static_cast<T>((*this)(min, max));
        };
        std::generate_n(output.begin(), size, gen);
		return output;
	}

    void SetSeed(uint64_t seed);

    Sfc64Engine<>& engine() {
        return engine_;
    }

    std::string GetRandomString(size_t size);    
private:
    Sfc64Engine<> engine_;
};

Sfc64Engine<> MakeRandomEngine();

XAMP_BASE_NAMESPACE_END
