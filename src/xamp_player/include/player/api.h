//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <player/iaudioplayer.h>
#include <stream/icddevice.h>

namespace xamp::player {

using namespace xamp::base;

XAMP_PLAYER_API void Xamp2Startup();

XAMP_PLAYER_API std::shared_ptr<IAudioPlayer> MakeAudioPlayer(const std::weak_ptr<IPlaybackStateAdapter>& adapter);

#ifdef XAMP_OS_WIN
XAMP_PLAYER_API AlignPtr<stream::ICDDevice>& OpenCD(int32_t driver_letter);
XAMP_PLAYER_API void CloseCD();
#endif

}
