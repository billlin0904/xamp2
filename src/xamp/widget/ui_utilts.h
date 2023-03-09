//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/playerorder.h>
#include <QSharedPointer>

class XMainWindow;
class XProgressDialog;
class ProcessIndicator;
class QFileDialog;

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

void MoveToTopWidget(QWidget* source_widget, const QWidget* target_widget);

XMainWindow* GetMainWindow();

PlayerOrder GetNextOrder(PlayerOrder cur) noexcept;

AlignPtr<IAudioProcessor> MakeR8BrainSampleRateConverter();

AlignPtr<IAudioProcessor> MakeSoxrSampleRateConverter(const QVariantMap& settings);

PlaybackFormat GetPlaybackFormat(IAudioPlayer* player);

QString GetExistingDirectory(QWidget* parent = nullptr,
    const QString& caption = QString(),
    const QString& directory = QString(),
    const QString& filter = QString());