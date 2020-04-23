//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSettings>
#include <QScopedPointer>

const QLatin1String SOXR_RESAMPLE_SAMPLRATE{ "resampleSampleRate" };
const QLatin1String SOXR_ENABLE_STEEP_FILTER{ "enableSteepFilter" };
const QLatin1String SOXR_QUALITY{ "quality" };
const QLatin1String SOXR_PHASE{ "phase" };
const QLatin1String SOXR_PASS_BAND{ "passBand" };
const QLatin1String SOXR_DEFAULT_SETTING_NAME{ "default" };

class JsonSettings {
public:
    static void loadJsonFile(const QString& file_name);

    static QVariant getValue(const QString& key);

    static int32_t getAsInt(const QString& key);

    static void remove(const QString& key);

    template <typename T>
    static void setDefaultValue(const QString& key, T value) {
        default_settings_[key] = value;
    }

    static void setValue(const QString& key, QVariant value) {
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

