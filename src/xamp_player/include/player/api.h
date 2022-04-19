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

XAMP_PLAYER_API void Initialize();

XAMP_PLAYER_API void Uninitialize();

struct XAMP_PLAYER_API XampIniter {
	XampIniter() {
	}

	void Init() {
		Initialize();
		success = true;
	}

	~XampIniter() {
		if (success) {
			Uninitialize();
		}
	}
private:
	bool success{false};
};	

XAMP_PLAYER_API std::shared_ptr<IAudioPlayer> MakeAudioPlayer(const std::weak_ptr<IPlaybackStateAdapter>& adapter);

#ifdef XAMP_OS_WIN
XAMP_PLAYER_API AlignPtr<ICDDevice> OpenCD(int32_t driver_letter);
#endif

}
