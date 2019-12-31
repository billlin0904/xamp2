//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ostream>

namespace xamp::player {

enum class PlayerState {
	PLAYER_STATE_RUNNING,
	PLAYER_STATE_PAUSED,
	PLAYER_STATE_RESUME,
	PLAYER_STATE_STOPPED
};

inline std::ostream& operator<<(std::ostream& ostr, PlayerState state) {
	switch (state) {
	case PlayerState::PLAYER_STATE_RUNNING:
		ostr << "Running";
		break;
	case PlayerState::PLAYER_STATE_PAUSED:
		ostr << "Paused";
		break;
	case PlayerState::PLAYER_STATE_RESUME:
		ostr << "Resume";
		break;
	case PlayerState::PLAYER_STATE_STOPPED:
		ostr << "Stopped";
		break;
	}
	return ostr;
}

}
