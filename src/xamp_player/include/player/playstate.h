//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>

namespace xamp::player {

XAMP_MAKE_ENUM(PlayerState,
		  PLAYER_STATE_INIT,
	      PLAYER_STATE_RUNNING,
	      PLAYER_STATE_PAUSED,
          PLAYER_STATE_RESUME,
          PLAYER_STATE_STOPPED,
          PLAYER_STATE_USER_STOPPED)

}
