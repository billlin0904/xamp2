#pragma once
#include <algorithm>
#include <cstdint>
#include <span>

namespace xamp::output_device::win32 {
// Converter input is stereo byte-interleaved MSB-first DSD.
inline bool copyNativeDsd(std::span<const uint8_t> input, std::span<uint8_t> left,
    std::span<uint8_t> right, bool lsb) {
    if(input.size()%2 || input.size()/2>left.size() || left.size()!=right.size()) return false;
    const auto reverse=[](uint8_t b) {
        b=static_cast<uint8_t>(((b&0x55)<<1)|((b>>1)&0x55));
        b=static_cast<uint8_t>(((b&0x33)<<2)|((b>>2)&0x33));
        return static_cast<uint8_t>((b<<4)|(b>>4));
    };
    const uint8_t idle=lsb ? 0x96 : 0x69;
    std::fill(left.begin(),left.end(),idle);std::fill(right.begin(),right.end(),idle);
    for(size_t i=0;i<input.size()/2;i++) {
        left[i]=lsb ? reverse(input[i*2]) : input[i*2];
        right[i]=lsb ? reverse(input[i*2+1]) : input[i*2+1];
    }
    return true;
}
}
