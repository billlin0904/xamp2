//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <random>
#include <algorithm>

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API RNG {
public:
    static RNG& Instance();

    XAMP_DISABLE_COPY(RNG)

	int32_t GetRandomInt() noexcept {
		return std::uniform_int_distribution(0, INT_MAX)(engine_);
	}

	template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type* = nullptr>
	T operator()(T min, T max) noexcept {
		return std::uniform_real_distribution(min, max)(engine_);
	}

    template <typename T, typename std::enable_if<std::is_integral<T>::value, float>::type* = nullptr>
	T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution(min, max)(engine_);
    }

	template <typename T>
	auto GetShuffle(const T& items) {
		std::vector<typename T::value_type> shuffled(items.begin(), items.end());
		std::shuffle(shuffled.begin(), shuffled.end(), engine_);
		return shuffled;
	}

	template <typename T>
	void Shuffle(T& items) {
		std::shuffle(items.begin(), items.end(), engine_);
	}
private:
    RNG() noexcept;

	std::mt19937_64 engine_;
};

}