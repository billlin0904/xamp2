#include <output_device/osx/coreaudiointeger.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <limits>

using namespace xamp::output_device::osx::integer_audio;
using namespace xamp::pcm;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

int main() {
    try {
        AudioStreamBasicDescription physical{176400, kAudioFormatLinearPCM,
            kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked, 6, 1, 6, 2, 24, 0};
        const auto packed24 = pcmFormat(physical);
        require(packed24 && packed24->container_bits == 24, "packed 24-bit stereo accepted");
        require(lossless(canonical(24, 2, 176400), *packed24), "DoP carrier is lossless");
        require(!lossless(canonical(32, 2, 176400), *packed24), "32-bit PCM must not truncate");
        require(!lossless(canonical(24, 2, 352800), *packed24), "carrier rate must match");
        for (auto flag : {kAudioFormatFlagIsFloat, kAudioFormatFlagIsNonInterleaved}) {
            auto unsupported = physical; unsupported.mFormatFlags |= flag;
            require(!pcmFormat(unsupported), "float and planar formats rejected");
        }
        auto invalid = physical; invalid.mFormatFlags = 0;
        require(!pcmFormat(invalid), "unsigned format rejected");
        invalid = physical; invalid.mChannelsPerFrame = 6;
        require(!pcmFormat(invalid), "multichannel layout rejected");
        invalid = physical; invalid.mBytesPerFrame = 7;
        require(!pcmFormat(invalid), "invalid frame stride rejected");
        invalid = physical; invalid.mSampleRate = std::numeric_limits<double>::quiet_NaN();
        require(!pcmFormat(invalid), "NaN sample rate rejected");
        invalid = physical; invalid.mBitsPerChannel = 65560;
        require(!pcmFormat(invalid), "oversized bit depth rejected before narrowing");
        invalid = physical; invalid.mSampleRate = 192000;
        require(!sameFormat(physical, invalid), "verification detects changed sample rate");

        // USB HAL drivers may list mixable integer physical formats first, while
        // exposing only non-mixable integers virtually (e.g. the connected KA5).
        auto nonmixable = physical;
        nonmixable.mFormatFlags |= kAudioFormatFlagIsNonMixable;
        std::array<AudioStreamRangedDescription, 2> hardware{{
            {physical, {176400, 176400}}, {nonmixable, {176400, 176400}}}};
        std::array<AudioStreamRangedDescription, 1> virtual_formats{{{nonmixable, {176400, 176400}}}};
        auto selected = selectFormat(hardware, virtual_formats, canonical(24, 2, 176400));
        require(selected && sameFormat(*selected, nonmixable), "negotiate format common to physical and virtual HAL");
        require(!selectFormat(hardware, virtual_formats, canonical(24, 2, 352800)), "unsupported carrier rejected");
        virtual_formats[0].mFormat.mFormatFlags = kAudioFormatFlagIsFloat;
        require(!selectFormat(hardware, virtual_formats, canonical(24, 2, 176400)), "float-only virtual output rejected");

        // Two data frames plus an idle tail; both channels must share a marker.
        std::array<std::byte, 24> canonical_dop{};
        canonical_dop[1] = std::byte{0x12}; canonical_dop[2] = std::byte{0x34};
        canonical_dop[5] = std::byte{0x56}; canonical_dop[6] = std::byte{0x78};
        uint8_t marker = 0x05;
        frameDop(canonical_dop, 2, marker);
        require(marker == 0xfa, "odd callback size advances marker phase");
        require(canonical_dop[3] == std::byte{5} && canonical_dop[7] == std::byte{5}, "same-frame markers match");
        require(canonical_dop[11] == std::byte{0xfa}, "markers alternate across data frames");
        require(canonical_dop[17] == std::byte{0x69} && canonical_dop[18] == std::byte{0x69}, "partial EOF uses DSD idle");
        std::array<std::byte, 18> output24{};
        std::span<std::byte> out{output24};
        require(convert({canonical_dop, 3, canonical(24, 2, 176400)}, *packed24, {&out, 1}), "24-bit DoP pack");
        require(output24[0] == std::byte{0x12} && output24[1] == std::byte{0x34} && output24[2] == std::byte{5}, "24-bit DoP payload and marker exact");
        require(output24[3] == std::byte{0x56} && output24[4] == std::byte{0x78}, "right channel payload preserved");

        // Test endian/alignment combinations used by 24-in-32 hardware, and full 32-bit.
        for (bool big : {false, true}) for (bool left : {false, true}) for (unsigned bits : {24u, 32u}) {
            auto format = physical;
            format.mBitsPerChannel = bits; format.mBytesPerFrame = format.mBytesPerPacket = 8;
            format.mFormatFlags = kAudioFormatFlagIsSignedInteger |
                (big ? kAudioFormatFlagIsBigEndian : 0) | (left ? kAudioFormatFlagIsAlignedHigh : 0);
            const auto target = pcmFormat(format);
            require(target.has_value(), "valid 32-bit container accepted");
            std::array<std::byte, 24> encoded{};
            std::span<std::byte> destination{encoded};
            require(convert({canonical_dop, 3, canonical(24, 2, 176400)}, *target, {&destination, 1}), "integer conversion succeeded");
            for (size_t sample = 0; sample < 6; ++sample) {
                auto expected = readSignedWord(canonical_dop.data() + sample * 4, canonical(24, 2, 176400));
                expected <<= bits - 24;
                require(readSignedWord(encoded.data() + sample * 4, *target) == expected, "all DoP payload bits survive packing");
            }
        }
        std::array<std::byte, 8> stopped{};
        frameDop(stopped, 0, marker);
        require(stopped[3] == std::byte{0xfa} && stopped[7] == std::byte{0xfa} && marker == 5,
            "idle callback preserves phase across buffer boundary");
        require(stopped[1] == std::byte{0x69} && stopped[5] == std::byte{0x69}, "stopped output emits DSD idle on both channels");
        std::cout << "CoreAudio integer format and DoP packing tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
