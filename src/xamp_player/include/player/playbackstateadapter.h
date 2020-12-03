//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>
#include <base/align_ptr.h>
#include <player/player.h>
#include <stream/filestream.h>
#include <output_device/devicestatelistener.h>
#include <player/playstate.h>

namespace xamp::player {

using namespace base;
using namespace stream;
using namespace output_device;

using GaplessPlayEntry = std::pair<AlignPtr<FileStream>, AlignPtr<Resampler>>;

class XAMP_PLAYER_API XAMP_NO_VTABLE PlaybackStateAdapter {
public:
    friend class AudioPlayer;

    virtual ~PlaybackStateAdapter() = default;

	virtual void OnError(Exception const & ex) = 0;

	virtual void OnStateChanged(PlayerState play_state) = 0;

	virtual void OnSampleTime(double stream_time) = 0;

	virtual void OnDeviceChanged(DeviceState state) = 0;

	virtual void OnVolumeChanged(float vol) = 0;

    virtual void OnSampleDataChanged(const float *samples, size_t size) = 0;

    virtual void OnGaplessPlayback() = 0;

    virtual size_t GetPlayQueueSize() const = 0;

    virtual GaplessPlayEntry& PlayQueueFont() = 0;

    virtual void PopPlayQueue() = 0;
    
protected:
    PlaybackStateAdapter() = default;

    virtual void ClearPlayQueue() = 0;
};

}
