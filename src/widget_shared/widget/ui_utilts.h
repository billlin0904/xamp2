//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/playerorder.h>
#include <QSharedPointer>

class XMainWindow;
class XProgressDialog;
class ProcessIndicator;
class QFileDialog;

struct XAMP_WIDGET_SHARED_EXPORT PlaybackFormat {
    bool is_dsd_file{ false };
    bool enable_sample_rate_convert{ false };
    uint32_t dsd_speed{ 0 };
    uint32_t bit_rate{ 0 };
    DsdModes dsd_mode{ DsdModes::DSD_MODE_PCM };
    AudioFormat file_format;
    AudioFormat output_format;
};

QSharedPointer<ProcessIndicator> MakeProcessIndicator(QWidget* widget);

void CenterDesktop(QWidget* widget);

XAMP_WIDGET_SHARED_EXPORT void CenterParent(QWidget* widget);

void MoveToTopWidget(QWidget* source_widget, const QWidget* target_widget);

XMainWindow* GetMainWindow();

XAMP_WIDGET_SHARED_EXPORT QString FormatSampleRate(const AudioFormat& format);

XAMP_WIDGET_SHARED_EXPORT QString Format2String(const PlaybackFormat& playback_format, const QString& file_ext);

XAMP_WIDGET_SHARED_EXPORT QSharedPointer<XProgressDialog> MakeProgressDialog(QString const& title,
    QString const& text, 
    QString const& cancel,
    QWidget* parent = nullptr);

XAMP_WIDGET_SHARED_EXPORT PlayerOrder GetNextOrder(PlayerOrder cur) noexcept;

XAMP_WIDGET_SHARED_EXPORT AlignPtr<IAudioProcessor> MakeR8BrainSampleRateConverter();

XAMP_WIDGET_SHARED_EXPORT AlignPtr<IAudioProcessor> MakeSoxrSampleRateConverter(const QVariantMap& settings);

XAMP_WIDGET_SHARED_EXPORT PlaybackFormat GetPlaybackFormat(IAudioPlayer* player);

XAMP_WIDGET_SHARED_EXPORT QString GetFileDialogFileExtensions();

XAMP_WIDGET_SHARED_EXPORT QString GetExistingDirectory(QWidget* parent);

XAMP_WIDGET_SHARED_EXPORT void GetOpenMusicFileName(QWidget* parent, std::function<void(const QString&)>&& action);

XAMP_WIDGET_SHARED_EXPORT void GetOpenFileName(QWidget* parent,
    std::function<void(const QString&)>&& action,
    const QString& caption = QString(),
    const QString& dir = QString(),
    const QString& filter = QString());

XAMP_WIDGET_SHARED_EXPORT void GetSaveFileName(QWidget* parent,
    std::function<void(const QString&)>&& action,
    const QString& caption = QString(),
    const QString& dir = QString(),
    const QString& filter = QString());

XAMP_WIDGET_SHARED_EXPORT void Delay(int32_t seconds);