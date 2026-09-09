#include <output_device/win32/wasapipcmformat.h>
#include <output_device/win32/asiopcmformat.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
using namespace xamp::pcm;
TEST_CASE("ASIO integer sample types describe the SDK alignment and endianness", "[asio]") {
    for (auto type:{ASIOSTInt16LSB,ASIOSTInt24LSB,ASIOSTInt32LSB,ASIOSTInt16MSB,ASIOSTInt24MSB,ASIOSTInt32MSB,
                   ASIOSTInt32LSB16,ASIOSTInt32LSB18,ASIOSTInt32LSB20,ASIOSTInt32LSB24,
                   ASIOSTInt32MSB16,ASIOSTInt32MSB18,ASIOSTInt32MSB20,ASIOSTInt32MSB24}) {
        const auto format=asioFormat(type,48000,2);REQUIRE(format.has_value());
        REQUIRE(format->layout==Layout::Planar);REQUIRE(valid(*format));
    }
    const auto right=asioFormat(ASIOSTInt32LSB24,48000,2).value();
    REQUIRE(right.container_bits==32);REQUIRE(right.valid_bits==24);
    REQUIRE(right.alignment==Alignment::Right);REQUIRE(right.byte_order==ByteOrder::Little);
    const auto full=asioFormat(ASIOSTInt32MSB,48000,2).value();
    REQUIRE(full.valid_bits==32);REQUIRE(full.byte_order==ByteOrder::Big);
    REQUIRE(lossless(canonical(32,2,48000),full));
    REQUIRE_FALSE(lossless(canonical(32,2,48000),right));
    REQUIRE_FALSE(asioFormat(ASIOSTFloat32LSB,48000,2).has_value());
    REQUIRE_FALSE(asioFormat(ASIOSTFloat64MSB,48000,2).has_value());
    REQUIRE_FALSE(asioFormat(ASIOSTDSDInt8LSB1,48000,2).has_value());
}
TEST_CASE("ASIO right-aligned buffers preserve positive and sign-extended negative LSBs", "[asio]") {
    std::array<std::byte,8> input{std::byte{0},std::byte{1},std::byte{0},std::byte{0},
        std::byte{0},std::byte{255},std::byte{255},std::byte{255}};
    std::array<std::byte,4> left{},right{};
    std::array<std::span<std::byte>,2> output{left,right};
    const auto format=asioFormat(ASIOSTInt32LSB24,44100,2).value();
    REQUIRE(convert({input,1,canonical(24,2,44100)},format,output));
    REQUIRE(std::to_integer<int>(left[0])==1); REQUIRE(std::to_integer<int>(left[3])==0);
    for (auto b:right) REQUIRE(std::to_integer<int>(b)==255);
}

TEST_CASE("WASAPI carries container depth independently of valid integer precision", "[wasapi]") {
    auto pcm=canonical(24,2,96000);
    const auto padded=wasapiFormat(pcm);REQUIRE(padded.has_value());
    REQUIRE(padded->Format.wBitsPerSample==32);REQUIRE(padded->Samples.wValidBitsPerSample==24);
    REQUIRE(padded->Format.nBlockAlign==8);REQUIRE(padded->Format.nAvgBytesPerSec==768000);
    REQUIRE(padded->Format.wFormatTag==WAVE_FORMAT_EXTENSIBLE);REQUIRE(padded->SubFormat.Data1==1);
    pcm.container_bits=24;const auto packed=wasapiFormat(pcm);REQUIRE(packed.has_value());
    REQUIRE(packed->Format.nBlockAlign==6);REQUIRE(packed->Format.wBitsPerSample==24);
    pcm=canonical(32,2,44100);const auto full=wasapiFormat(pcm);REQUIRE(full.has_value());
    REQUIRE(full->Samples.wValidBitsPerSample==32);REQUIRE(full->Format.nSamplesPerSec==44100);
    pcm.alignment=Alignment::Right;REQUIRE_FALSE(wasapiFormat(pcm).has_value());
    pcm=canonical(16,2,44100);pcm.container_bits=16;
    const auto narrow=wasapiFormat(pcm);REQUIRE(narrow.has_value());
    REQUIRE(narrow->Format.wFormatTag==WAVE_FORMAT_PCM);REQUIRE(narrow->Format.nBlockAlign==4);
}
