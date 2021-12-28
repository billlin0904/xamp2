//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

namespace xamp::stream {

struct XAMP_STREAM_API CompressorParameters final {
    CompressorParameters() noexcept
        : gain(-1)
        , threshold(-1)
        , ratio(100)
        , attack(0.1f)
        , release(50) {
    }

    // 你想被壓縮後的整條音軌增強多少音量
    // Output gain in dB of signal after compression.
    float gain;
    // Point in dB at which compression begins, in decibels, in the range from -60 to 0.
    // 音量門檻，告訢壓縮器何時開啟壓縮程序
    float threshold;
    // Compression ratio, in the range from 1 to 100.
    // 壓縮時的強弱
    float ratio;
    // Time in ms before compression reaches its full value, in the range from 0.01 to 500.
    // 當音量超過Threshold後，你想壓縮器什麼時候開始壓縮？
    float attack;
    // Time (speed) in ms at which compression is stopped after input drops below fThreshold, in the range from 50 to 3000.
    // 當聲音被壓縮後而音量又回落低至Threshold，你想壓縮器什麼時候停止壓縮？
    float release;
};

}