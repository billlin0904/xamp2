//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_PALYER_API_EXPORTS
#define XAMP_PALYER_API __declspec(dllexport)
#else
#define XAMP_PALYER_API __declspec(dllimport)
#endif

#include <cstdint>

namespace xamp::player {
	class AudioPlayer;
	class PlaybackStateAdapter;
}


