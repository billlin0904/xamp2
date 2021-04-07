//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API XAMP_NO_VTABLE SampleWriter {
public:
	virtual ~SampleWriter();

	virtual bool TryWrite(float const* sample, size_t num_samples) = 0;
protected:
	SampleWriter();
};

}
