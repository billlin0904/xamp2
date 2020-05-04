//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/id.h>

#include <QSettings>
#include <QScopedPointer>

#include <widget/localelanguage.h>

extern const QLatin1String kAppSettingLang;
extern const QLatin1String kAppSettingPreventSleep;

extern const QLatin1String kAppSettingDeviceType;
extern const QLatin1String kAppSettingDeviceId;
extern const QLatin1String kAppSettingWidth;
extern const QLatin1String kAppSettingHeight;
extern const QLatin1String kAppSettingVolume;
extern const QLatin1String kAppSettingOrder;
extern const QLatin1String kAppSettingNightMode;
extern const QLatin1String kAppSettingEnableBlur;
extern const QLatin1String kAppSettingBackgroundColor;
extern const QLatin1String kAppSettingMusicFilePath;

extern const QLatin1String kAppSettingResamplerEnable;
extern const QLatin1String kAppSettingSoxrSettingName;

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

    static void setValue(const QLatin1String& key, const QLatin1String& value) {
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

    static void loadLanguage(const QString& lang);

    static void setOrDefaultConfig();

    static void save();

protected:
    AppSettings() = default;

private:
    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
    static LocaleLanguageManager manager_;
};
