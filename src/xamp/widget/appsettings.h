//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

#include <QColor>
#include <QSettings>
#include <QScopedPointer>

#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/settingnames.h>
#include <widget/localelanguage.h>

struct AppEQSettings {
    float gain{0};
    float Q{0};
};
Q_DECLARE_METATYPE(AppEQSettings);

class FramelessWindow;

class AppSettings final {
public:    
    static void loadIniFile(QString const & file_name);

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
    static void setValue(QString const& key, T value) {
        setValue(key, std::to_string(value));
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
    static void setValue(char const *key, T value) {
        setValue(QLatin1String(key), std::to_string(value));
    }

    static void setValue(QString const & key, QColor value) {
        settings_->setValue(key, value.name(QColor::HexArgb));
    }

    static void setValue(QString const & key, QVariant value) {
        settings_->setValue(key, value);
    }

    static void setValue(QString const & key, std::string value) {
        settings_->setValue(key, QString::fromStdString(value));
    }

    static void setValue(QString const & key, std::wstring value) {
        settings_->setValue(key, QString::fromStdWString(value));
    }

    static void setValue(QString const & key, QString const & value) {
        settings_->setValue(key, value);
    }

    static void setValue(QLatin1String const & key, QLatin1String const & value) {
        settings_->setValue(key, value);
    }

    template <typename T>
    static void setDefaultValue(const QString& key, T value) {
        default_settings_[key] = value;
    }

    static Uuid getID(QString const & key);

    static QSize getSizeValue(QString const& width_key, QString const& height_key);

    static QVariant getValue(QString const& key);

    static int32_t getAsInt(QString const& key);

    static bool getValueAsBool(QString const& key) {
        return getValue(key).toBool();
    }

    static QString getValueAsString(QString const& key) {
        return getValue(key).toString();
    }

    static bool contains(QString const& key) {
        return settings_->contains(key);
    }

    static QList<QString> getList(QString const& key);

    static void removeList(QString const& key, QString const & value);

    static void addList(QString const& key, QString const & value);

    static void loadLanguage(QString const& lang);

    static QString getMyMusicFolderPath();

    static void save();

protected:
    AppSettings() = default;

private:
    static void loadEQPreset();

    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
    static LocaleLanguageManager manager_;;
};
