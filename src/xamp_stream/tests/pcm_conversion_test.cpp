#include <base/pcm.h>
#include <base/bitperfect.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>
#include <bit>
using namespace xamp::pcm;
namespace {
std::vector<std::byte> bytes(std::initializer_list<uint32_t> words) {
    std::vector<std::byte> result;
    for (auto w:words) for (unsigned b=0;b<4;++b) result.push_back(static_cast<std::byte>((w>>(8*b))&255));
    return result;
}
bool encode(const std::vector<std::byte>& data, Format from, Format to, std::span<std::byte> output) {
    return convert({data,data.size()/from.frameBytes(),from},to,{&output,1});
}
}
TEST_CASE("All 16-bit integer samples survive canonical byte conversion", "[pcm]") {
    auto from=canonical(16,1,44100); auto to=from;to.container_bits=16;
    for (int value=-32768;value<=32767;++value) {
        const auto input=bytes({static_cast<uint32_t>(value)<<16});
        std::array<std::byte,2> actual;
        REQUIRE(encode(input,from,to,actual));
        REQUIRE(std::to_integer<unsigned>(actual[0]) == (static_cast<unsigned>(value)&255));
        REQUIRE(std::to_integer<unsigned>(actual[1]) == ((static_cast<unsigned>(value)>>8)&255));
    }
}
TEST_CASE("True 32-bit integers retain extrema and low bits in direct copies", "[pcm]") {
    auto fmt=canonical(32,1,96000);
    const auto input=bytes({0,1,0xffffffff,0x7fffffff,0x80000000,0x01000001,0x80000001,0x7ffffffe});
    std::vector<std::byte> actual(input.size());
    REQUIRE(encode(input,fmt,fmt,actual));
    REQUIRE((actual==input));
    auto narrow=fmt;narrow.valid_bits=24;
    REQUIRE_FALSE(encode(input,fmt,narrow,actual));
    narrow.valid_bits=16;narrow.container_bits=16;
    REQUIRE_FALSE(encode(input,fmt,narrow,actual));
}
TEST_CASE("24-bit ASIO right alignment differs from WASAPI left alignment", "[pcm]") {
    const auto input=bytes({0x12345600,0xffffff00,0x80000000,0x00000100});
    auto source=canonical(24,1,48000);
    auto right=source;right.alignment=Alignment::Right;
    std::vector<std::byte> actual(input.size());
    REQUIRE(encode(input,source,right,actual));
    REQUIRE((actual==bytes({0x00123456,0xffffffff,0xff800000,0x00000001})));
    std::vector<std::byte> restored(input.size());
    REQUIRE(encode(actual,right,source,restored)); REQUIRE((restored==input));
}
TEST_CASE("Endian changes and channel separation preserve every independent sample", "[pcm]") {
    const auto input=bytes({0x12345678,0x87654321,0x00000001,0xffffffff});
    auto source=canonical(32,2,192000);auto target=source;
    target.byte_order=ByteOrder::Big;target.layout=Layout::Planar;
    std::array<std::byte,8> left{},right{};
    std::array<std::span<std::byte>,2> out{left,right};
    REQUIRE(convert({input,2,source},target,out));
    REQUIRE((left==std::array<std::byte,8>{std::byte{0x12},std::byte{0x34},std::byte{0x56},std::byte{0x78},std::byte{0},std::byte{0},std::byte{0},std::byte{1}}));
    REQUIRE((right==std::array<std::byte,8>{std::byte{0x87},std::byte{0x65},std::byte{0x43},std::byte{0x21},std::byte{255},std::byte{255},std::byte{255},std::byte{255}}));
    std::vector<std::byte> planes(left.begin(),left.end());planes.insert(planes.end(),right.begin(),right.end());
    std::vector<std::byte> restored(input.size());
    REQUIRE(encode(planes,target,source,restored)); REQUIRE((restored==input));
}
TEST_CASE("24-bit packed and both byte orders roundtrip deterministic random samples", "[pcm]") {
    uint32_t seed=0x12345678;std::vector<std::byte> input;
    for (unsigned i=0;i<20000;++i) {
        seed=seed*1664525u+1013904223u;
        const auto word=seed&0xffffff00;
        for (unsigned b=0;b<4;++b) input.push_back(static_cast<std::byte>((word>>(8*b))&255));
    }
    auto source=canonical(24,2,44100);
    for (auto order:{ByteOrder::Little,ByteOrder::Big}) {
        auto target=source;target.container_bits=24;target.byte_order=order;
        std::vector<std::byte> packed(input.size()/4*3),restored(input.size());
        REQUIRE(encode(input,source,target,packed));
        REQUIRE(encode(packed,target,source,restored)); REQUIRE((restored==input));
    }
}
TEST_CASE("Widening valid bits keeps the same amplitude and signed value", "[pcm]") {
    auto from=canonical(16,1,44100);auto to=canonical(24,1,44100);to.alignment=Alignment::Right;
    const auto input=bytes({0x7fff0000,0x80000000,0xffff0000,0x00010000});
    std::vector<std::byte> result(input.size());
    REQUIRE(encode(input,from,to,result));
    REQUIRE((result==bytes({0x007fff00,0xff800000,0xffffff00,0x00000100})));
}
TEST_CASE("Invalid PCM blocks and lossy format changes are rejected before writing", "[policy]") {
    auto source=canonical(32,2,44100); const auto input=bytes({1,2});
    std::array<std::byte,8> output{};auto target=source;
    target.sample_rate=48000;REQUIRE_FALSE(encode(input,source,target,output));
    target=source;target.channels=1;REQUIRE_FALSE(encode(input,source,target,output));
    target=source;target.type=SampleType::Float;REQUIRE_FALSE(encode(input,source,target,output));
    target=source;target.container_bits=0;REQUIRE_FALSE(encode(input,source,target,output));
    REQUIRE_FALSE(encode(input,source,source,std::span<std::byte>{output.data(),7}));
    std::span<std::byte> out{output};
    REQUIRE_FALSE(convert({input,2,source},source,{&out,1}));
    REQUIRE_FALSE(convert({input,1,source},source,{}));
    REQUIRE(xamp::bitperfect::supported(32,2,44100));
    REQUIRE_FALSE(xamp::bitperfect::supported(0,2,44100));
}
TEST_CASE("Final partial frames drain before EOF; underruns never replay old bytes", "[buffer]") {
    using namespace xamp::bitperfect;
    REQUIRE(classifyRead(1024,1024,8,false)==ReadStatus::Samples);
    REQUIRE(classifyRead(136,1024,8,true)==ReadStatus::Samples);
    REQUIRE(classifyRead(0,1024,8,true)==ReadStatus::End);
    REQUIRE(classifyRead(136,1024,8,false)==ReadStatus::Underrun);
    REQUIRE(classifyRead(0,1024,8,false)==ReadStatus::Underrun);
    REQUIRE(classifyRead(137,1024,8,true)==ReadStatus::Underrun);
}

TEST_CASE("Producer refills before large device watermark empties FIFO", "[buffering]") {
    constexpr size_t capacity=4096, block=1024, callback=256;
    size_t queued=capacity;
    for(int tick=0;tick<1000;++tick) {
        REQUIRE(queued>=callback);
        queued-=callback;
        while(xamp::bitperfect::canRefill(capacity-queued,block,block)) queued+=block;
    }
    REQUIRE_FALSE(xamp::bitperfect::canRefill(capacity,capacity+1,0));
    REQUIRE_FALSE(xamp::bitperfect::canRefill(block-1,block,0));
}
