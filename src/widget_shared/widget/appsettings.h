//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

#include <QColor>
#include <QDataStream>
#include <qfileinfo.h>
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
        for (const auto& band : object.settings.bands) {
            arch << band.type << band.frequency << band.gain << band.Q;
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
            arch >> object.settings.bands[i].type >> object.settings.bands[i].frequency >> object.settings.bands[i].gain >> object.settings.bands[i].Q;
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
    AppSettings() = default;

    void LoadIniFile(QString const & file_name);

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
    void SetValue(QString const& key, T value) {
        SetValue(key, std::to_string(value));
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
     void SetValue(char const *key, T value) {
        SetValue(QLatin1String(key), std::to_string(value));
    }

    template <typename T>
    void SetEnumValue(const QString& key, T value) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        SetValue<int32_t>(key, static_cast<int32_t>(value));
    }

    void SetValue(QString const& key, QColor value) const {
        settings_->setValue(key, value.name(QColor::HexArgb));
    }

    void SetValue(QString const & key, QByteArray value) const {
        settings_->setValue(key, value);
    }

    void SetValue(QString const & key, QVariant value) const {
        settings_->setValue(key, value);
    }

    void SetValue(QString const & key, const std::string value) const {
        settings_->setValue(key, QString::fromStdString(value));
    }

    void SetValue(QString const & key, const std::wstring value) const {
        settings_->setValue(key, QString::fromStdWString(value));
    }

    void SetValue(QString const & key, QString const & value) const {
        settings_->setValue(key, value);
    }

    void SetValue(QLatin1String const & key, QLatin1String const & value) const {
        settings_->setValue(key, value);
    }

    template <typename T>
    void SetDefaultValue(const QString& key, T value) {
        default_settings_[key] = value;
    }

    template <typename T>
    void SetDefaultEnumValue(const QString& key, T value) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        SetDefaultValue<int32_t>(key, static_cast<int32_t>(value));
    }

    QColor ValueAsColor(QString const& key, QColor default_color = Qt::white) {
        if (!Contains(key)) {
            return default_color;
        }
        return QColor(ValueAsString(key));
    }

    Uuid ValueAsId(QString const & key);

    QSize ValueAsSize(QString const& width_key, QString const& height_key);

    QVariant GetValue(QString const& key);

    int32_t ValueAsInt(QString const& key);

    template <typename T>
    T ValueAsEnum(QString const& key) {
        static_assert(std::is_enum_v<T>, "T must be enum value");
        return static_cast<T>(ValueAsInt(key));
    }

    bool ValueAsBool(QString const& key) {
        return GetValue(key).toBool();
    }

    QString ValueAsString(QString const& key) {
        return GetValue(key).toString();
    }

    [[nodiscard]] bool Contains(QString const& key) const {
        XAMP_EXPECTS(settings_ != nullptr);
        return settings_->contains(key);
    }

    QList<QString> ValueAsStringList(QString const& key);

    void RemoveList(QString const& key, QString const & value);

    void AddList(QString const& key, QString const & value);

    void LoadLanguage(QString const& lang);

    QString GetMyMusicFolderPath();

    QString DefaultCachePath();

    void save();

    const QMap<QString, EqSettings>& GetEqPreset();

    AppEQSettings GetEqSettings();

    void SetEqSettings(AppEQSettings const &eq_settings);

    void AddDontShowMeAgain(const QString &text);

    void ParseFixedBandEQ(const QFileInfo file_info, QFile& file);

    void ParseGraphicEQ(const QFileInfo file_info, QFile& file);

    bool DontShowMeAgain(const QString& text);

    QLocale locale();

    void LoadSoxrSetting();

    void LoadR8BrainSetting();

    void SaveLogConfig();

    void LoadOrSaveLogConfig();

    void LoadAppSettings();
private:
    void RegisterMetaType();

    void LoadEqPreset();

    QScopedPointer<QSettings> settings_;
    QMap<QString, QVariant> default_settings_;
    LocaleLanguageManager manager_;
    QMap<QString, EqSettings> eq_settings_;
};

#define qAppSettings SharedSingleton<AppSettings>::GetInstance()