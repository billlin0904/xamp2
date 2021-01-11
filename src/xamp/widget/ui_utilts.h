//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <player/audio_player.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>

struct PlaybackFormat {
    bool is_dsd_file{ false };
    bool enable_sample_rate_convert{ false };
    uint32_t dsd_speed{ 0 };
    DsdModes dsd_mode{ DsdModes::DSD_MODE_PCM };
    AudioFormat file_format;
    AudioFormat output_format;
};

static QString dsdSampleRate2String(uint32_t dsd_speed) {
    return Q_STR("%1 MHz").arg((dsd_speed / 64) * 2.8);
}

static QString samplerate2String(const AudioFormat& format) {
    auto precision = 1;
    auto is_mhz_samplerate = false;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_samplerate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }

    return (is_mhz_samplerate ? QString::number(format.GetSampleRate() / double(1000000), 'f', 2) + Q_UTF8("MHz/")
        : QString::number(format.GetSampleRate() / double(1000), 'f', precision) + Q_UTF8("kHz/"));
}

static QString format2String(const PlaybackFormat &playback_format, const QString& file_ext) {
    auto format = playback_format.file_format;

    auto ext = file_ext;
    ext = ext.remove(Q_UTF8(".")).toUpper();

    auto precision = 1;
    auto is_mhz_sample_rate = false;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }

    auto bits = format.GetBitsPerSample();
    if (is_mhz_sample_rate) {
        bits = 1;
    }

    QString dsd_speed_format;
    if (playback_format.is_dsd_file) {
        dsd_speed_format = Q_UTF8(" DSD") + QString::number(playback_format.dsd_speed) + Q_UTF8(" ");
    }

    QString output_format_str;
    QString dsd_mode;
    QString bit_format;

    switch (playback_format.dsd_mode) {
    case DsdModes::DSD_MODE_PCM:
        dsd_mode = Q_UTF8("PCM");
        output_format_str = samplerate2String(playback_format.output_format);
        bit_format = QString::number(bits) + Q_UTF8("bit");
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = Q_UTF8("Native DSD");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = Q_UTF8("DOP");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    }

    return ext
        + Q_UTF8(" | ")
        + dsd_speed_format
        + output_format_str
        + bit_format
        + Q_UTF8(" | ") + dsd_mode;
}
