//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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
	* onError is called when an error occurs.
	*/
	virtual void onError(const std::exception & ex) = 0;

	/*
	* outputFormatChanged is called when the output format changed.
	*/
	virtual void outputFormatChanged(const AudioFormat output_format, size_t buffer_size) = 0;

	/*
	* onStateChanged is called when the player state changed.
	* 
	*/
	virtual void onStateChanged(PlayerState play_state) = 0;

	/*
	* onSampleTime is called when the sample time changed.
	* 
	*/
	virtual void onSampleTime(double stream_time) = 0;

	/*
	* onDeviceChanged is called when the device changed.
	*/
	virtual void onDeviceChanged(DeviceState state, const std::string & device_id) = 0;

	/*
	* onVolumeChanged is called when the volume changed.	
	*/
	virtual void onVolumeChanged(int32_t vol) = 0;

	/*
	* onSamplesChanged is called when the samples changed.	
	*/
	virtual void onSamplesChanged(const float* samples, size_t num_samples) = 0;

protected:
	IPlaybackStateAdapter() = default;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
