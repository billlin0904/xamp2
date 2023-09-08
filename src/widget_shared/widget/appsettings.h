//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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
#include <widget/widget_shared_global.h>
#include <widget/localelanguage.h>

struct XAMP_WIDGET_SHARED_EXPORT AppEQSettings {
    QString name;
    EqSettings settings;

    friend QDataStream& operator << (QDataStream& arch, const AppEQSettings& object) {
        arch.setFloatingPointPrecision(QDataStream::SinglePrecision);
        arch << object.name;
        arch << object.settings.preamp;
        arch << static_cast<quint32>(object.settings.bands.size());
        for (size_t i = 0; i < object.settings.bands.size(); ++i) {
            arch << object.settings.bands[i].frequency << object.settings.bands[i].gain << object.settings.bands[i].Q;
        }
        return arch;
    }

    friend QDataStream& operator >> (QDataStream& arch, AppEQSettings& object) {
        arch.setFloatingPointPrecision(QDataStream::SinglePrecision);
        arch >> object.name;
        arch >> object.settings.preamp;
        int total = 0;
        arch >> total;
        object.settings.bands.resize(total);
        for (auto i = 0; i < total; ++i) {
            arch >> object.settings.bands[i].frequency >> object.settings.bands[i].gain >> object.settings.bands[i].Q;
        }
        return arch;
    }
};
Q_DECLARE_METATYPE(AppEQSettings);

enum class ReplayGainMode {
	RG_ALBUM_MODE,
    RG_TRACK_MODE,
    RG_NONE_MODE,
};

enum class ReplayGainScanMode {
    RG_SCAN_MODE_FAST,
    RG_SCAN_MODE_FULL,
};

class XAMP_WIDGET_SHARED_EXPORT AppSettings final {
public:    
    static void LoadIniFile(QString const & file_name);

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
    static void SetValue(QString const& key, T value) {
        SetValue(key, std::to_string(value));
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
    static void SetValue(char const *key, T value) {
        SetValue(QLatin1String(key), std::to_string(value));
    }

    template <typename T>
    static void SetEnumValue(const QString& key, T value) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        SetValue<int32_t>(key, static_cast<int32_t>(value));
    }

    static void SetValue(QString const& key, QColor value) {
        settings_->setValue(key, value.name(QColor::HexArgb));
    }

    static void SetValue(QString const & key, QByteArray value) {
        settings_->setValue(key, value);
    }

    static void SetValue(QString const & key, QVariant value) {
        settings_->setValue(key, value);
    }

    static void SetValue(QString const & key, const std::string value) {
        settings_->setValue(key, QString::fromStdString(value));
    }

    static void SetValue(QString const & key, const std::wstring value) {
        settings_->setValue(key, QString::fromStdWString(value));
    }

    static void SetValue(QString const & key, QString const & value) {
        settings_->setValue(key, value);
    }

    static void SetValue(QLatin1String const & key, QLatin1String const & value) {
        settings_->setValue(key, value);
    }

    template <typename T>
    static void SetDefaultValue(const QString& key, T value) {
        default_settings_[key] = value;
    }

    template <typename T>
    static void SetDefaultEnumValue(const QString& key, T value) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        SetDefaultValue<int32_t>(key, static_cast<int32_t>(value));
    }

    static QColor ValueAsColor(QString const& key, QColor default_color = Qt::white) {
        if (!contains(key)) {
            return default_color;
        }
        return QColor(ValueAsString(key));
    }

    static Uuid ValueAsId(QString const & key);

    static QSize ValueAsSize(QString const& width_key, QString const& height_key);

    static QVariant GetValue(QString const& key);

    static int32_t ValueAsInt(QString const& key);

    template <typename T>
    static T ValueAsEnum(QString const& key) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        return static_cast<T>(ValueAsInt(key));
    }

    static bool ValueAsBool(QString const& key) {
        return GetValue(key).toBool();
    }

    static QString ValueAsString(QString const& key) {
        return GetValue(key).toString();
    }

    static bool contains(QString const& key) {
        return settings_->contains(key);
    }

    static QList<QString> ValueAsStringList(QString const& key);

    static void RemoveList(QString const& key, QString const & value);

    static void AddList(QString const& key, QString const & value);

    static void LoadLanguage(QString const& lang);

    static QString GetMyMusicFolderPath();

    static QString DefaultCachePath();

    static void save();

    static const QMap<QString, EqSettings>& GetEqPreset();

    static AppEQSettings GetEqSettings();

    static void SetEqSettings(AppEQSettings const &eq_settings);

    static void AddDontShowMeAgain(const QString &text);

    static bool DontShowMeAgain(const QString& text);

    static QLocale locale();

    static void LoadSoxrSetting();

    static void LoadR8BrainSetting();

    static void SaveLogConfig();

    static void LoadOrSaveLogConfig();

    static void LoadAppSettings();
protected:
    AppSettings() = default;

private:
    static void RegisterMetaType();

    static void LoadEqPreset();

    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
    static LocaleLanguageManager manager_;
    static QMap<QString, EqSettings> eq_settings_;
};
