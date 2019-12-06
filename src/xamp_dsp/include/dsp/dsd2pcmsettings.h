//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <vector>

#include <dsp/dsp.h>

namespace xamp::dsp {

using CoefsTable = std::vector<std::array<float, 256>>;

struct CoefsFirTable {
	int32_t fir_length{ 0 };
	CoefsTable ctable;
};

class XAMP_DSP_API Dsd2PcmSettings {
public:
	static Dsd2PcmSettings& Instance() {
		static Dsd2PcmSettings setting;
		return setting;
	}

	CoefsFirTable GetDsdFir1_64() const;

private:
	Dsd2PcmSettings();
};

}


