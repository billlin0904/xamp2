//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <player/audio_player.h>
#include <widget/widget_shared.h>
#include <thememanager.h>

class XMainWindow;
class XProgressDialog;
class ProcessIndicator;

struct PlaybackFormat {
    bool is_dsd_file{ false };
    bool enable_sample_rate_convert{ false };
    uint32_t dsd_speed{ 0 };
    uint32_t bit_rate{ 0 };
    DsdModes dsd_mode{ DsdModes::DSD_MODE_PCM };
    AudioFormat file_format;
    AudioFormat output_format;
};

QString FormatSampleRate(const AudioFormat& format);

QString Format2String(const PlaybackFormat& playback_format, const QString& file_ext);

QSharedPointer<ProcessIndicator> MakeProcessIndicator(QWidget* widget);

QSharedPointer<XProgressDialog> MakeProgressDialog(QString const& title,
    QString const& text, 
    QString const& cancel,
    QWidget* parent = nullptr);

void CenterDesktop(QWidget* widget);

void CenterParent(QWidget* widget);

void CenterTarget(QWidget* source_widget, const QWidget* target_widget);

XMainWindow* GetMainWindow();
