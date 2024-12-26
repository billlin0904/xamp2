//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <base/enum.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(PlayerState,
		  PLAYER_STATE_INIT,
	      PLAYER_STATE_RUNNING,
	      PLAYER_STATE_PAUSED,
          PLAYER_STATE_RESUME,
          PLAYER_STATE_STOPPED,
          PLAYER_STATE_USER_STOPPED)

XAMP_AUDIO_PLAYER_NAMESPACE_END
