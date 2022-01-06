//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

#include <QColor>
#include <QDataStream>
#include <QSettings>
#include <QScopedPointer>

#include <stream/eqsettings.h>
#include <widget/widget_shared.h>
#include <widget/localelanguage.h>
#include <widget/str_utilts.h>
#include <widget/settingnames.h>

struct AppEQSettings {
    QString name;
    EQSettings settings;

    friend QDataStream& operator << (QDataStream& arch, const AppEQSettings& object) {
        arch.setFloatingPointPrecision(QDataStream::SinglePrecision);
        arch << object.name;
        arch << object.settings.preamp;
        arch << static_cast<quint32>(object.settings.bands.size());
        for (auto i = 0; i < object.settings.bands.size(); ++i) {
            arch << object.settings.bands[i].gain << object.settings.bands[i].Q;
        }
        return arch;
    }

    friend QDataStream& operator >> (QDataStream& arch, AppEQSettings& object) {
        arch.setFloatingPointPrecision(QDataStream::SinglePrecision);
        arch >> object.name;
        arch >> object.settings.preamp;
        int total = 0;
        arch >> total;
        for (auto i = 0; i < total; ++i) {
            arch >> object.settings.bands[i].gain >> object.settings.bands[i].Q;
        }
        return arch;
    }
};
Q_DECLARE_METATYPE(AppEQSettings);

enum ReplayGainMode {
	RG_ALBUM_MODE,
    RG_TRACK_MODE,
    RG_NONE_MODE,
};

enum ReplayGainScanMode {
    RG_SCAN_MODE_FAST,
    RG_SCAN_MODE_FULL,
};

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

    template <typename T>
    static void setEnumValue(const QString& key, T value) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        setValue<int32_t>(key, static_cast<int32_t>(value));
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

    template <typename T>
    static void setDefaultEnumValue(const QString& key, T value) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        setDefaultValue<int32_t>(key, static_cast<int32_t>(value));
    }

    static Uuid getID(QString const & key);

    static QSize getSizeValue(QString const& width_key, QString const& height_key);

    static QVariant getValue(QString const& key);

    static int32_t getAsInt(QString const& key);

    template <typename T>
    static T getAsEnum(QString const& key) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        return static_cast<T>(getAsInt(key));
    }

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

    static const QMap<QString, EQSettings>& getEQPreset();

protected:
    AppSettings() = default;

private:
    static void loadEQPreset();

    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
    static LocaleLanguageManager manager_;
    static QMap<QString, EQSettings> eq_settings_;
};
