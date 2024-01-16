//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

struct XAMP_BASE_API EncodingProfile {
	EncodingProfile();

	uint32_t bitrate = 0;
	uint32_t num_channels = 0;
	uint32_t bit_per_sample = 0;
	uint32_t sample_rate = 0;
	uint32_t bytes_per_second = 0;
};

XAMP_BASE_NAMESPACE_END
