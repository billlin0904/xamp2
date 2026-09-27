#pragma once
#include <widget/appsettings.h>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace pcm_dsd_settings {
inline const auto enabledKey = QStringLiteral("pcm2DsdEnabled");
inline const auto multiplierKey = QStringLiteral("pcm2DsdMultiplier");
inline const auto gainKey = QStringLiteral("pcm2DsdGainDb");
inline const auto ditherKey = QStringLiteral("pcm2DsdDither");
inline const auto filterKey = QStringLiteral("pcm2DsdFilter");
inline const auto modulatorKey = QStringLiteral("pcm2DsdModulator");
inline const auto blockKey = QStringLiteral("pcm2DsdBlockFrames");
inline const auto transportKey = QStringLiteral("pcm2DsdTransport");
struct Settings {
    bool enabled{false};
    uint32_t multiplier{64};
    double gain_db{0.0};
    bool dither{true};
    uint32_t transport{0}; // 0: DoP, 1: ASIO Native DSD
    uint32_t filter{0};
    uint32_t modulator{0};
    uint32_t block_frames{256};
};
inline Settings load() {
    Settings s;
    s.enabled = qAppSettings.valueAsBool(enabledKey);
    if (qAppSettings.contains(multiplierKey)) s.multiplier = qAppSettings.valueAs(multiplierKey).toUInt();
    if (qAppSettings.contains(gainKey)) s.gain_db = qAppSettings.valueAs(gainKey).toDouble();
    if (qAppSettings.contains(ditherKey)) s.dither = qAppSettings.valueAsBool(ditherKey);
    if (qAppSettings.contains(filterKey)) s.filter = qAppSettings.valueAs(filterKey).toUInt();
    if (qAppSettings.contains(modulatorKey)) s.modulator = qAppSettings.valueAs(modulatorKey).toUInt();
    if (qAppSettings.contains(blockKey)) s.block_frames = qAppSettings.valueAs(blockKey).toUInt();
    if (qAppSettings.contains(transportKey)) s.transport = qAppSettings.valueAs(transportKey).toUInt();
    return s;
}
inline void validate(const Settings& s) {
    if (s.transport > 1 || s.filter > 2 || s.modulator > 1 || s.block_frames < 64 || s.block_frames > 1024 ||
        (s.block_frames & (s.block_frames - 1)))
        throw std::runtime_error("Invalid PCM2DSD filter, modulator or block size");
    if (s.multiplier < 16 || s.multiplier > 2048 || (s.multiplier & (s.multiplier - 1)) ||
        !std::isfinite(s.gain_db) || s.gain_db < -24.0 || s.gain_db > 12.0)
        throw std::runtime_error("Invalid PCM2DSD settings; choose a DSD rate and gain between -24 and +12 dB");
}
}
