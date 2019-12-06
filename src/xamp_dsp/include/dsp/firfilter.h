//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <dsp/dsp.h>

namespace xamp::dsp {

class XAMP_DSP_API FirFilter {
public:
	virtual ~FirFilter() = default;

protected:
	FirFilter() = default;
};

}
