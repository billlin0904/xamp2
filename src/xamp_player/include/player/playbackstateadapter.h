//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>
#include <player/player.h>
#include <output_device/devicestatelistener.h>
#include <player/playstate.h>

namespace xamp::player {

using namespace base;
using namespace stream;
using namespace output_device;

class XAMP_PLAYER_API XAMP_NO_VTABLE PlaybackStateAdapter {
public:
    friend class AudioPlayer;

    virtual ~PlaybackStateAdapter() = default;

	virtual void OnError(Exception const & ex) = 0;

	virtual void OnStateChanged(PlayerState play_state) = 0;

	virtual void OnSampleTime(double stream_time) = 0;

	virtual void OnDeviceChanged(DeviceState state) = 0;

	virtual void OnVolumeChanged(float vol) = 0;

protected:
    PlaybackStateAdapter() = default;
};

}
