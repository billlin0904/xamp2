//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/id.h>

#include <QSettings>
#include <QScopedPointer>

#include <widget/str_utilts.h>
#include <widget/localelanguage.h>

const ConstLatin1String kAppSettingLang{ "AppSettings/lang" };

const ConstLatin1String kAppSettingPreventSleep{ "AppSettings/preventSleep" };
const ConstLatin1String kAppSettingDeviceType{ "AppSettings/deviceType" };
const ConstLatin1String kAppSettingDeviceId{ "AppSettings/deviceId" };
const ConstLatin1String kAppSettingWidth{ "AppSettings/width" };
const ConstLatin1String kAppSettingHeight{ "AppSettings/height" };
const ConstLatin1String kAppSettingVolume{ "AppSettings/volume" };
const ConstLatin1String kAppSettingOrder{ "AppSettings/order" };
const ConstLatin1String kAppSettingNightMode{ "AppSettings/nightMode" };
const ConstLatin1String kAppSettingBackgroundColor{ "AppSettings/theme/backgroundColor" };
const ConstLatin1String kAppSettingEnableBlur{ "AppSettings/theme/enableBlur" };
const ConstLatin1String kAppSettingMusicFilePath{ "AppSettings/musicFilePath" };

const ConstLatin1String kAppSettingResamplerEnable{ "AppSettings/soxr/enable" };
const ConstLatin1String kAppSettingSoxrSettingName{ "AppSettings/soxr/userSettingName" };

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

    static QString getMyMusicFolderPath();

    static void save();

protected:
    AppSettings() = default;

private:
    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
    static LocaleLanguageManager manager_;
};
