//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>
#include <player/player.h>
#include <player/playstate.h>

namespace xamp::player {

using namespace base;

class XAMP_PALYER_API XAMP_NO_VTABLE PlaybackStateAdapter {
public:
	XAMP_BASE_CLASS(PlaybackStateAdapter)

	virtual void OnError(const Exception& ex) = 0;

	virtual void OnStateChanged(PlayerState play_state) = 0;

	virtual void OnSampleTime(double stream_time) = 0;

protected:
	PlaybackStateAdapter() = default;
};

}