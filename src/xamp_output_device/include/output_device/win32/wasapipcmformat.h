#pragma once
#include <windows.h>
#include <mmreg.h>
#include <base/pcm.h>
#include <optional>
namespace xamp::pcm {
inline std::optional<WAVEFORMATEXTENSIBLE> wasapiFormat(const Format& f) {
    if (!valid(f) || f.channels != 2 || f.byte_order != ByteOrder::Little ||
        f.alignment != Alignment::Left || f.layout != Layout::Interleaved ||
        f.sample_rate > UINT32_MAX/f.frameBytes()) return std::nullopt;
    WAVEFORMATEXTENSIBLE wave{};
    wave.Format.wFormatTag = f.container_bits == 16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_EXTENSIBLE;
    wave.Format.nChannels = f.channels;
    wave.Format.nSamplesPerSec = f.sample_rate;
    wave.Format.nBlockAlign = static_cast<WORD>(f.frameBytes());
    wave.Format.nAvgBytesPerSec = static_cast<DWORD>(f.frameBytes()*f.sample_rate);
    wave.Format.wBitsPerSample = f.container_bits;
    wave.Format.cbSize = f.container_bits == 16 ? 0 : sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
    wave.Samples.wValidBitsPerSample = f.valid_bits;
    wave.dwChannelMask = 3; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT
    wave.SubFormat = {0x00000001,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}}; // PCM
    return wave;
}
}
