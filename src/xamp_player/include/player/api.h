//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <player/iaudioplayer.h>
#include <player/iplaybackstateadapter.h>
#include <stream/icddevice.h>

namespace xamp::player {

XAMP_PLAYER_API void LoadComponentSharedLibrary();

XAMP_PLAYER_API std::shared_ptr<IAudioPlayer> MakeAudioPlayer();

#ifdef XAMP_OS_WIN
XAMP_PLAYER_API AlignPtr<ICDDevice> OpenCD(int32_t driver_letter);
#endif

}
