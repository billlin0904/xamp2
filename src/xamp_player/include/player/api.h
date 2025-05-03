//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <player/iaudioplayer.h>
#include <player/iplaybackstateadapter.h>
#include <stream/icddevice.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

/*
* Load the shared library of the audio player component.
*/
XAMP_PLAYER_API void LoadComponentSharedLibrary();

/*
* Create an audio player instance.
*/
XAMP_PLAYER_API std::shared_ptr<IAudioPlayer> MakeAudioPlayer();

#ifdef XAMP_OS_WIN
/*
* Create cd device instance.
*/
XAMP_PLAYER_API ScopedPtr<ICDDevice> OpenCD(int32_t driver_letter);
#endif

XAMP_AUDIO_PLAYER_NAMESPACE_END
