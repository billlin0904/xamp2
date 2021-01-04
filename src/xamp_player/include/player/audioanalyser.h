#pragma once

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API AudioAnalyser final {
public:
    AudioAnalyser(int32_t frame_size, int32_t sample_rate);

    ~AudioAnalyser();

    void Process(float const * samples, uint32_t num_sample);

    float GetRMS();

    float GetPeakEnergy();

    float GetZeroCrossingRate();

private:
    class AudioAnalyserImpl;
    AlignPtr<AudioAnalyserImpl> impl_;
};

}

