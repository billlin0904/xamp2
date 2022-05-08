//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>
#include <player/player.h>
#include <output_device/idevicestatelistener.h>
#include <player/playstate.h>

namespace xamp::player {

class XAMP_PLAYER_API XAMP_NO_VTABLE IPlaybackStateAdapter {
public:
	XAMP_BASE_CLASS(IPlaybackStateAdapter)

	virtual void OnError(Exception const & ex) = 0;

	virtual void OnOutputFormatChanged(const AudioFormat output_format) = 0;

	virtual void OnStateChanged(PlayerState play_state) = 0;

	virtual void OnSampleTime(double stream_time) = 0;

	virtual void OnDeviceChanged(DeviceState state) = 0;

	virtual void OnVolumeChanged(float vol) = 0;

	virtual void OnSamplesChanged(const float* samples, size_t num_buffer_frames) = 0;

protected:
	IPlaybackStateAdapter() = default;
};

}
