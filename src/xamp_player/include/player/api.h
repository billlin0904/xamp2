//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <player/iaudioplayer.h>
#include <player/iplaybackstateadapter.h>
#include <stream/icddevice.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

XAMP_PLAYER_API void loadComponentSharedLibrary();

XAMP_PLAYER_API void unloadComponentSharedLibrary();

XAMP_PLAYER_API std::shared_ptr<IAudioPlayer> makeAudioPlayer();

#ifdef XAMP_OS_WIN
XAMP_PLAYER_API ScopedPtr<ICDDevice> openCD(int32_t driver_letter);
#endif

XAMP_AUDIO_PLAYER_NAMESPACE_END
