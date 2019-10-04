//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

namespace xamp::base {

enum class DSDSampleFormat {
	DSD_INT8LSB,
	DSD_INT8MSB, 
	DSD_INT8NER8
};

enum class DSDModes {
	DSD_MODE_PCM,
	DSD_MODE_DOP,
	DSD_MODE_DOP_AA,
	DSD_MODE_RAW
};

}
