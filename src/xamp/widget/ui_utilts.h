//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <player/audio_player.h>
#include <widget/widget_shared.h>
#include <thememanager.h>

#include <QMessageBox>
#include <QApplication>

class QProgressDialog;
class ProcessIndicator;

struct PlaybackFormat {
    bool is_dsd_file{ false };
    bool enable_sample_rate_convert{ false };
    uint32_t dsd_speed{ 0 };
    uint32_t bitrate{ 0 };
    DsdModes dsd_mode{ DsdModes::DSD_MODE_PCM };
    AudioFormat file_format;
    AudioFormat output_format;
};

QString sampleRate2String(const AudioFormat& format);

QString format2String(const PlaybackFormat& playback_format, const QString& file_ext);

QSharedPointer<ProcessIndicator> makeProcessIndicator(QWidget* widget);

QScopedPointer<QProgressDialog> makeProgressDialog(QString const& title, QString const& text, QString const& cancel);

std::tuple<bool, QMessageBox::StandardButton> showDontShowAgainDialog(bool show_agin);

void centerDesktop(QWidget* widget);

void centerParent(QWidget* widget);

