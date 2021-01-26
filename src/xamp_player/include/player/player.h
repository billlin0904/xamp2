//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#ifdef XAMP_PALYER_API_EXPORTS
#define XAMP_PLAYER_API __declspec(dllexport)
#else
#define XAMP_PLAYER_API __declspec(dllimport)
#endif
#else
#define XAMP_PLAYER_API
#endif

namespace xamp::player {
	class AudioPlayer;	
	class PlaybackStateAdapter;
	class Chromaprint;
	class SampleRateConverter;
    class SoxrSampleRateConverter;
    class PassThroughSampleRateConverter;
	class LoudnessScanner;
}


