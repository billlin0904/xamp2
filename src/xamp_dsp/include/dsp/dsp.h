//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef _WIN32
#ifdef XAMP_DSP_API_EXPORTS
#define XAMP_DSP_API __declspec(dllexport)
#else
#define XAMP_DSP_API __declspec(dllimport)
#endif
#else
#define XAMP_DSP_API
#endif

#include <cstdint>

namespace xamp::dsp {
}
