//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <player/playstate.h>

#include <output_device/idevicestatelistener.h>

#include <base/base.h>
#include <base/exception.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

class XAMP_PLAYER_API XAMP_NO_VTABLE IPlaybackStateAdapter {
public:
	XAMP_BASE_CLASS(IPlaybackStateAdapter)

	virtual void OnError(const Exception & ex) = 0;

	virtual void OutputFormatChanged(const AudioFormat output_format, size_t buffer_size) = 0;

	virtual void OnStateChanged(PlayerState play_state) = 0;

	virtual void OnSampleTime(double stream_time) = 0;

	virtual void OnDeviceChanged(DeviceState state, const std::string & device_id) = 0;

	virtual void OnVolumeChanged(int32_t vol) = 0;

	virtual void OnSamplesChanged(const float* samples, size_t num_buffer_frames) = 0;

protected:
	IPlaybackStateAdapter() = default;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
