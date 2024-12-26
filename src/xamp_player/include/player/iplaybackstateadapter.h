//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>
#include <player/playstate.h>

#include <output_device/idevicestatelistener.h>

#include <base/base.h>
#include <base/exception.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

/*
* IPlaybackStateAdapter is an player state adapter interface.
* 
*/
class XAMP_PLAYER_API XAMP_NO_VTABLE IPlaybackStateAdapter {
public:
	XAMP_BASE_CLASS(IPlaybackStateAdapter)

    /*
	* OnError is called when an error occurs.
	*/
	virtual void OnError(const std::exception & ex) = 0;

	/*
	* OutputFormatChanged is called when the output format changed.
	*/
	virtual void OutputFormatChanged(const AudioFormat output_format, size_t buffer_size) = 0;

	/*
	* OnStateChanged is called when the player state changed.
	* 
	*/
	virtual void OnStateChanged(PlayerState play_state) = 0;

	/*
	* OnSampleTime is called when the sample time changed.
	* 
	*/
	virtual void OnSampleTime(double stream_time) = 0;

	/*
	* OnDeviceChanged is called when the device changed.
	*/
	virtual void OnDeviceChanged(DeviceState state, const std::string & device_id) = 0;

	/*
	* OnVolumeChanged is called when the volume changed.	
	*/
	virtual void OnVolumeChanged(int32_t vol) = 0;

	/*
	* OnSamplesChanged is called when the samples changed.	
	*/
	virtual void OnSamplesChanged(const float* samples, size_t num_buffer_frames) = 0;

protected:
	IPlaybackStateAdapter() = default;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
