#include <output_device/win32/wasapipcmformat.h>
#include <output_device/win32/asiopcmformat.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>
using namespace xamp::pcm;
TEST_CASE("ASIO widens 16 and 24-bit PCM into 32-bit containers without changing amplitude", "[asio][bitperfect]") {
    for (const unsigned bits : {16u, 24u}) {
        // Exhaust all 16-bit values; exercise 24-bit extrema, low bits and a reproducible sample set.
        std::vector<uint32_t> samples{0, 1, (1u << (bits-1))-1, 1u << (bits-1), (1u << bits)-1};
        uint32_t seed = 0x12345678;
        const uint32_t mask = (1u << bits)-1;
        for (uint32_t i=0; i<65536; ++i) {
            seed = seed*1664525u+1013904223u;
            samples.push_back(bits == 16 ? i : seed & mask);
        }
        const size_t frames = samples.size();
        std::vector<std::byte> input(frames*8);
        for (size_t f=0; f<frames; ++f) for (size_t ch=0; ch<2; ++ch) {
            const auto sample = samples[ch == 0 ? f : frames-1-f];
            const uint32_t word = sample << (32-bits);
            for (size_t b=0; b<4; ++b) input[(f*2+ch)*4+b] = static_cast<std::byte>(word >> (b*8));
        }
        for (const auto type : {ASIOSTInt32LSB, ASIOSTInt32MSB, ASIOSTInt32LSB24, ASIOSTInt32MSB24}) {
            CAPTURE(bits, type);
            const auto target = asioFormat(type, 96000, 2).value();
            std::vector<std::byte> left(frames*4), right(frames*4);
            std::array<std::span<std::byte>,2> output{left,right};
            REQUIRE(lossless(canonical(bits,2,96000),target));
            REQUIRE(convert({input,frames,canonical(bits,2,96000)},target,output));
            const bool big = type == ASIOSTInt32MSB || type == ASIOSTInt32MSB24;
            const unsigned valid_bits = type == ASIOSTInt32LSB || type == ASIOSTInt32MSB ? 32 : 24;
            // Independent expected bytes: scale the signed source, then serialize its two's complement.
            for (size_t f=0; f<frames; ++f) for (size_t ch=0; ch<2; ++ch) {
                const uint32_t raw = samples[ch == 0 ? f : frames-1-f];
                const int64_t signed_sample = raw & (1u << (bits-1)) ? int64_t(raw)-(int64_t{1} << bits) : raw;
                const uint32_t expected = static_cast<uint32_t>(signed_sample*(int64_t{1} << (valid_bits-bits)));
                for (size_t b=0; b<4; ++b)
                    REQUIRE(output[ch][f*4+b] == static_cast<std::byte>(expected >> (8*(big ? 3-b : b))));
            }
        }
    }
}
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
