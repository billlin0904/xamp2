//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <player/player.h>

namespace xamp::base {

template <size_t N, typename StateType>
class DspChain {
public:
	template <typename Filter, typename Sample>
	void Process(Sample const * samples, uint32_t num_samples, Filter& filter) {
		for (auto state : states_) {
			filter.process(num_samples, samples, state);
		}
	}	
private:
	std::array<StateType, N> states_{0};
};

template <typename StateType>
class DspChain<0, StateType> {
public:
	template <typename Filter, typename Sample>
	void Process(Sample const* samples, uint32_t num_samples, Filter& filter) {
	}
};
	
}

