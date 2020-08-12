//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <player/audio_player.h>
#include <widget/str_utilts.h>

static QString samplerate2String(const xamp::base::AudioFormat& format) {
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

static QString format2String(const xamp::player::AudioPlayer* player, const QString& file_ext) {
    auto format = player->GetFileFormat();

    auto ext = file_ext;
    ext = ext.remove(Q_UTF8(".")).toUpper();

    auto precision = 1;
    auto is_mhz_samplerate = false;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_samplerate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }

    auto bits = format.GetBitsPerSample();
    if (is_mhz_samplerate) {
        bits = 1;
    }

    QString dsd_speed_format;
    if (player->IsDSDFile()) {
        auto dsd_speed = player->GetDSDSpeed();
        dsd_speed_format = Q_UTF8(" DSD") + QString::number(dsd_speed.value()) + Q_UTF8(" ");
    }

    QString dsd_mode;
    switch (player->GetDsdModes()) {
    case xamp::base::DsdModes::DSD_MODE_PCM:
        dsd_mode = Q_UTF8("PCM");
        break;
    case xamp::base::DsdModes::DSD_MODE_NATIVE:
        dsd_mode = Q_UTF8("Native DSD");
        break;
    case xamp::base::DsdModes::DSD_MODE_DOP:
        dsd_mode = Q_UTF8("DOP");
        break;
    }

    QString output_format_str;
    auto output_format = player->GetOutputFormat();
    if (format.GetFormat() != xamp::base::DataFormat::FORMAT_DSD && output_format != format) {
        output_format_str = samplerate2String(output_format);
    }

    return ext
        + Q_UTF8(" | ")
        + dsd_speed_format
        + samplerate2String(format) + output_format_str
        + QString::number(bits) + Q_UTF8("bit")
        + Q_UTF8(" | ") + dsd_mode;
}
