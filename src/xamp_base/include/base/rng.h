//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

    uint64_t operator()() noexcept {
        return engine_();
    }

    size_t Next(size_t max = (std::numeric_limits<size_t>::max)()) noexcept {
        return (*this)(size_t{0}, max);
    }

    uint32_t NextUInt32(
        const uint32_t min = (std::numeric_limits<uint32_t>::min)(),
        const uint32_t max = (std::numeric_limits<uint32_t>::max)())  noexcept {
        return (*this)(min, max);
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
        const float min = 0.0f,
        const float max = 1.0f) noexcept {
        return (*this)(min, max);
    }

    std::vector<int8_t> NextBytes(size_t size,
        const int32_t min = (std::numeric_limits<int8_t>::min)(),
        const int32_t max = (std::numeric_limits<int8_t>::max)()) {
        std::vector<int8_t> output(size);
        const auto gen = [this, min, max]() noexcept {
            return static_cast<int8_t>(NextInt32(min, max));
        };
        std::generate_n(output.begin(), size, gen);
        return output;
    }

    template <typename T>
	std::vector<T> NextBytes(size_t size,
		const T min = (std::numeric_limits<T>::min)(),
		const T max = (std::numeric_limits<T>::max)()) {        
        std::vector<T> output(size);
        const auto gen = [this, min, max]() noexcept {
            return static_cast<T>((*this)(min, max));
        };
        std::generate_n(output.begin(), size, gen);
		return output;
	}

    std::vector<float> NextSingles(size_t size, 
        const float min = 0.0f,
        const float max = 1.0f) {
        std::vector<float> output(size);
		const auto gen = [this, min, max]() noexcept {
			return NextSingle(min, max);
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

XAMP_BASE_NAMESPACE_END
