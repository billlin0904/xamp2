#pragma once
#include <base/pcm.h>
#include <asio.h>
#include <optional>
namespace xamp::pcm {
// ASIO SDK 2.3, pp.33-34: Int32LSB/MSBxx are RIGHT aligned and sign extended.
inline std::optional<Format> asioFormat(ASIOSampleType type, unsigned rate, unsigned channels) {
    Format f{rate,static_cast<uint16_t>(channels),0,0};
    f.layout = Layout::Planar;
    switch(type) {
    case ASIOSTInt16LSB: f.valid_bits=16; f.container_bits=16; break;
    case ASIOSTInt24LSB: f.valid_bits=24; f.container_bits=24; break;
    case ASIOSTInt32LSB: f.valid_bits=32; f.container_bits=32; break;
    case ASIOSTInt16MSB: f.valid_bits=16; f.container_bits=16; f.byte_order=ByteOrder::Big; break;
    case ASIOSTInt24MSB: f.valid_bits=24; f.container_bits=24; f.byte_order=ByteOrder::Big; break;
    case ASIOSTInt32MSB: f.valid_bits=32; f.container_bits=32; f.byte_order=ByteOrder::Big; break;
    case ASIOSTInt32LSB16: case ASIOSTInt32LSB18: case ASIOSTInt32LSB20: case ASIOSTInt32LSB24:
        f.container_bits=32; f.alignment=Alignment::Right;
        f.valid_bits=type==ASIOSTInt32LSB16?16:type==ASIOSTInt32LSB18?18:type==ASIOSTInt32LSB20?20:24;
        break;
    case ASIOSTInt32MSB16: case ASIOSTInt32MSB18: case ASIOSTInt32MSB20: case ASIOSTInt32MSB24:
        f.container_bits=32; f.alignment=Alignment::Right; f.byte_order=ByteOrder::Big;
        f.valid_bits=type==ASIOSTInt32MSB16?16:type==ASIOSTInt32MSB18?18:type==ASIOSTInt32MSB20?20:24;
        break;
    default: return std::nullopt; // Float and DSD require a different playback contract.
    }
    return f;
}
}
