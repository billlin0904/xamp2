//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

namespace xamp::base {

enum class DsdSampleFormat {
	DSD_INT8LSB,
	DSD_INT8MSB, 
	DSD_INT8NER8
};

enum DsdModes : uint8_t {
	DSD_MODE_PCM = 0,
	DSD_MODE_NATIVE = 2,
	DSD_MODE_DOP = 4,
	// NOTE: Unsupported now!
	// DSD_MODE_DOP_AA = 8
};

}
