//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API ChannelMixer {
public:
    ChannelMixer();

    void Process(float const *samples, size_t num_sample, AudioBuffer<int8_t>& buffer);

private:
    Buffer<float> buffer_;
};

}
