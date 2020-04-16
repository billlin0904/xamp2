//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/id.h>

#include <QSize>
#include <QColor>
#include <QSettings>
#include <QScopedPointer>

extern const QLatin1String APP_SETTING_LANG;

extern const QLatin1String APP_SETTING_DEVICE_TYPE;
extern const QLatin1String APP_SETTING_DEVICE_ID;
extern const QLatin1String APP_SETTING_WIDTH;
extern const QLatin1String APP_SETTING_HEIGHT;
extern const QLatin1String APP_SETTING_VOLUME;
extern const QLatin1String APP_SETTING_ORDER;
extern const QLatin1String APP_SETTING_NIGHT_MODE;
extern const QLatin1String APP_SETTING_ENABLE_BLUR;
extern const QLatin1String APP_SETTING_BACKGROUND_COLOR;
extern const QLatin1String APP_SETTING_MUSIC_FILE_PATH;

extern const QLatin1String APP_SETTING_RESAMPLER_ENABLE;
extern const QLatin1String APP_SETTING_SOXR_RESAMPLE_SAMPLRATE;
extern const QLatin1String APP_SETTING_SOXR_ENABLE_STEEP_FILTER;
extern const QLatin1String APP_SETTING_SOXR_QUALITY;
extern const QLatin1String APP_SETTING_SOXR_PHASE;
extern const QLatin1String APP_SETTING_SOXR_PASS_BAND;

class AppSettings {
public:    
    static void loadIniFile(const QString& file_name);

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value, T>>
    static void setValue(const QString& key, T value) {
        setValue(key, std::to_string(value));
    }

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value, T>>
    static void setValue(const char *key, T value) {
        setValue(QLatin1String(key), std::to_string(value));
    }

    static void setValue(const QString& key, QColor value) {
        settings_->setValue(key, value.name(QColor::HexArgb));
    }

    static void setValue(const QString &key, QVariant value) {
        settings_->setValue(key, value);
    }

    static void setValue(const QString &key, std::string value) {
        settings_->setValue(key, QString::fromStdString(value));
    }

    static void setValue(const QString &key, std::wstring value) {
        settings_->setValue(key, QString::fromStdWString(value));
    }

    static void setValue(const QString &key, const QString &value) {
        settings_->setValue(key, value);
    }

    template <typename T>
    static void setDefaultValue(const QString& key, T value) {
        default_settings_[key] = value;
    }

    static xamp::base::ID getID(const QString& key);

    static QSize getSizeValue(const QString& width_key, const QString& height_key);

    static QVariant getValue(const QString& key);

    static int32_t getAsInt(const QString& key);

    static bool getValueAsBool(const QString& key) {
        return getValue(key).toBool();
    }

    static QString getValueAsString(const QString& key) {
        return getValue(key).toString();
    }

protected:
    AppSettings() = default;

private:
    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
};
