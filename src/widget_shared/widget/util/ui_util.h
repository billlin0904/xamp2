//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/playerorder.h>
#include <widget/xmessagebox.h>

#include <QSharedPointer>
#include <QTableView>
#include <QWidget>

class XApplication;
class IXMainWindow;
class QDir;
class XProgressDialog;
class ProcessIndicator;
class QFileDialog;
class QStyleOptionViewItem;
class QAbstractItemView;

inline constexpr std::string_view kCueFileExtension(".cue");
inline constexpr std::string_view kZipFileExtension(".zip");

struct XAMP_WIDGET_SHARED_EXPORT PlaybackFormat {
    bool is_dsd_file{ false };
    bool enable_sample_rate_convert{ false };
    uint32_t dsd_speed{ 0 };
    uint32_t bit_rate{ 0 };
    DsdModes dsd_mode{ DsdModes::DSD_MODE_PCM };
    AudioFormat file_format;
    AudioFormat output_format;
};

XAMP_WIDGET_SHARED_EXPORT QSharedPointer<ProcessIndicator> makeProcessIndicator(QWidget* widget);

XAMP_WIDGET_SHARED_EXPORT void centerDesktop(QWidget* widget);

XAMP_WIDGET_SHARED_EXPORT void moveToTopWidget(QWidget* source_widget, const QWidget* target_widget);

XAMP_WIDGET_SHARED_EXPORT void centerParent(QWidget* widget);

XAMP_WIDGET_SHARED_EXPORT IXMainWindow* getMainWindow();

XAMP_WIDGET_SHARED_EXPORT QString formatSampleRate(const AudioFormat& format);

XAMP_WIDGET_SHARED_EXPORT QString format2String(const PlaybackFormat& playback_format, const QString& file_ext);

XAMP_WIDGET_SHARED_EXPORT QSharedPointer<XProgressDialog> makeProgressDialog(QString const& title,
    QString const& text, 
    QString const& cancel,
    QWidget* parent = nullptr);

XAMP_WIDGET_SHARED_EXPORT PlayerOrder getNextOrder(PlayerOrder cur) noexcept;

XAMP_WIDGET_SHARED_EXPORT ScopedPtr<IAudioProcessor> makeR8BrainSampleRateConverter();

XAMP_WIDGET_SHARED_EXPORT ScopedPtr<IAudioProcessor> makeSrcSampleRateConverter();

XAMP_WIDGET_SHARED_EXPORT ScopedPtr<IAudioProcessor> makeSoxrSampleRateConverter(uint32_t sample_rate);

XAMP_WIDGET_SHARED_EXPORT ScopedPtr<IAudioProcessor> makeSoxrSampleRateConverter(const QVariantMap& settings);

XAMP_WIDGET_SHARED_EXPORT ScopedPtr<IAudioProcessor> makeSampleRateConverter(uint32_t sample_rate);

XAMP_WIDGET_SHARED_EXPORT PlaybackFormat getPlaybackFormat(IAudioPlayer* player);

XAMP_WIDGET_SHARED_EXPORT QString getFileDialogFileExtensions();

XAMP_WIDGET_SHARED_EXPORT QString getExistingDirectory(QWidget* parent,
    const QString& title = QString(),
    const std::function<void(const QString&)>& action = nullptr);

XAMP_WIDGET_SHARED_EXPORT void getOpenMusicFileName(QWidget* parent,
    const QString& title,
    const QString& files,
    std::function<void(const QString&)>&& action);

XAMP_WIDGET_SHARED_EXPORT void getOpenFileName(QWidget* parent,
    std::function<void(const QString&)>&& action,
    const QString& caption = QString(),
    const QString& dir = QString(),
    const QString& filter = QString());

XAMP_WIDGET_SHARED_EXPORT void getSaveFileName(QWidget* parent,
    std::function<void(const QString&)>&& action,
    const QString& caption = QString(),
    const QString& dir = QString(),
    const QString& filter = QString());

XAMP_WIDGET_SHARED_EXPORT void delay(int32_t seconds);

XAMP_WIDGET_SHARED_EXPORT QSharedPointer<XMessageBox> makeMessageBox(const QString& title,
    const QString &message, 
    QWidget* parent = nullptr);

XAMP_WIDGET_SHARED_EXPORT const QStringList& getTrackInfoFileNameFilter();

XAMP_WIDGET_SHARED_EXPORT size_t getFileCount(const QString& dir,
    const QStringList& file_name_filters = QStringList());

XAMP_WIDGET_SHARED_EXPORT bool isSupportFileExtension(const QString &file_ext);

XAMP_WIDGET_SHARED_EXPORT void setTabViewStyle(QTableView* table_view);;

XAMP_WIDGET_SHARED_EXPORT QList<QModelIndex> getVisibleIndexes(const QAbstractItemView* list_view, 
    int32_t column);

XAMP_WIDGET_SHARED_EXPORT QString uniqueFileName(const QDir& dir,
    const QString& originalName);

XAMP_WIDGET_SHARED_EXPORT QString getValidFileName(QString fileName);

XAMP_WIDGET_SHARED_EXPORT QString applicationPath();

XAMP_WIDGET_SHARED_EXPORT void showMeMessage(const QString& message);