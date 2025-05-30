//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(DsdTimes,
	DSD_TIME_3X = 3, // DSD8
	DSD_TIME_4X = 4, // DSD16
	DSD_TIME_5X,     // DSD32
	DSD_TIME_6X,	 // DSD64
	DSD_TIME_7X,	 // DSD128
	DSD_TIME_8X,
	DSD_TIME_9X,
	DSD_TIME_10X,
	DSD_TIME_11X)

XAMP_STREAM_NAMESPACE_END

