//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSettings>
#include <QScopedPointer>

#include <widget/str_utilts.h>

static constexpr ConstLatin1String kSoxrResampleSampleRate{ "resampler/soxr/resampleSampleRate" };
static constexpr ConstLatin1String kSoxrEnableSteepFilter{ "resampler/soxr/enableSteepFilter" };
static constexpr ConstLatin1String kSoxrQuality{ "resampler/soxr/quality" };
static constexpr ConstLatin1String kSoxrPhase{ "resampler/soxr/phase" };
static constexpr ConstLatin1String kSoxrPassBand{ "resampler/soxr/passBand" };
static constexpr ConstLatin1String kSoxrDefaultSettingName{ "resampler/soxr/default" };

class JsonSettings {
public:
    static void loadJsonFile(QString const& file_name);

    static QVariant getValue(QString const& key);

    static int32_t getAsInt(QString const& key);

    static void remove(const QString& key);

    template <typename T>
    static void setDefaultValue(QString const& key, T const & value) {
        default_settings_[key] = value;
    }

    static void setValue(QString const & key, QVariant const & value) {
        settings_->setValue(key, value);
    }

    static QStringList keys();

    static void save();

protected:
    JsonSettings() = default;

private:
    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
};

