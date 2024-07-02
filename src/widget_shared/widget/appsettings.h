//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

	friend QDataStream& operator <<(QDataStream& arch, const AppEQSettings& object) {
		arch.setFloatingPointPrecision(QDataStream::SinglePrecision);
		arch << object.name;
		arch << object.settings.preamp;
		arch << static_cast<quint32>(object.settings.bands.size());
		for (const auto& band : object.settings.bands) {
			arch << band.type
				<< band.frequency
				<< band.gain 
				<< band.Q
				<< band.shelf_slope;
		}
		//arch << object.name;
		return arch;
	}

	friend QDataStream& operator >>(QDataStream& arch, AppEQSettings& object) {
		arch.setFloatingPointPrecision(QDataStream::SinglePrecision);
		arch >> object.name;
		arch >> object.settings.preamp;
		quint32 total = 0;
		arch >> total;
		object.settings.bands.resize(total);
		for (auto i = 0; i < total; ++i) {
			arch >> object.settings.bands[i].type				
				>> object.settings.bands[i].frequency
				>> object.settings.bands[i].gain
				>> object.settings.bands[i].Q
				>> object.settings.bands[i].shelf_slope;
		}
		//arch >> object.name;
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

	void loadIniFile(const QString& file_name);

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
	void setValue(const QString& key, T value) {
		setValue(key, std::to_string(value));
	}

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
	void setValue(const char* key, T value) {
		SetValue(QLatin1String(key), std::to_string(value));
	}

	template <typename T>
	void setEnumValue(const QString& key, T value) {
		static_assert(std::is_enum_v<T>, "T must be enum value");
		setValue<int32_t>(key, static_cast<int32_t>(value));
	}

	void setValue(const QString& key, QColor value) const {
		settings_->setValue(key, value.name(QColor::HexArgb));
	}

	void setValue(const QString& key, QByteArray value) const {
		settings_->setValue(key, value);
	}

	void setValue(const QString& key, QVariant value) const {
		settings_->setValue(key, value);
	}

	void setValue(const QString& key, const std::string value) const {
		settings_->setValue(key, QString::fromStdString(value));
	}

	void setValue(const QString& key, const std::wstring value) const {
		settings_->setValue(key, QString::fromStdWString(value));
	}

	void setValue(const QString& key, const QString& value) const {
		settings_->setValue(key, value);
	}

	void setValue(const QLatin1String& key, const QLatin1String& value) const {
		settings_->setValue(key, value);
	}

	template <typename T>
	void setDefaultValue(const QString& key, T value) {
		default_settings_[key] = value;
	}

	template <typename T>
	void setDefaultEnumValue(const QString& key, T value) {
		static_assert(std::is_enum_v<T>, "T must be enum value");
		setDefaultValue<int32_t>(key, static_cast<int32_t>(value));
	}

	QColor valueAsColor(const QString& key, QColor default_color = Qt::white) {
		if (!contains(key)) {
			return default_color;
		}
		return QColor(valueAsString(key));
	}

	Uuid valueAsId(const QString& key);

	QSize valueAsSize(const QString& width_key, const QString& height_key);

	QVariant valueAs(const QString& key);

	int32_t valueAsInt(const QString& key);

	template <typename T>
	T valueAsEnum(const QString& key) {
		static_assert(std::is_enum_v<T>, "T must be enum value");
		return static_cast<T>(valueAsInt(key));
	}

	bool valueAsBool(const QString& key) {
		return valueAs(key).toBool();
	}

	QString valueAsString(const QString& key) {
		return valueAs(key).toString();
	}

	[[nodiscard]] bool contains(const QString& key) const {
		XAMP_EXPECTS(settings_ != nullptr);
		return settings_->contains(key);
	}

	QList<QString> valueAsStringList(const QString& key);

	void removeList(const QString& key, const QString& value);

	void addList(const QString& key, const QString& value);

	void loadLanguage();

	void loadLanguage(const QString& lang);

	QString myMusicFolderPath();

	QString getOrCreateCachePath();

	void save();

	QLocale locale() const;

	const QMap<QString, EqSettings>& eqPreset();

	AppEQSettings eqSettings();

	void setEqSettings(const AppEQSettings& eq_settings);

	void addDontShowMeAgain(const QString& text);

	void parseEQPreset(QFileInfo file_info, QFile& file);

	void parseGraphicEq(QFileInfo file_info, QFile& file);

	bool dontShowMeAgain(const QString& text);

	QLocale locale();

	void loadSoxrSetting();

	void LoadR8BrainSetting();

	void saveLogConfig();

	void loadOrSaveLogConfig();

	void loadAppSettings();

private:
	void registerMetaType();

	void loadEqPreset();

	QScopedPointer<QSettings> settings_;
	QMap<QString, QVariant> default_settings_;
	LocaleLanguageManager manager_;
	QMap<QString, EqSettings> eq_settings_;
};

#define qAppSettings SharedSingleton<AppSettings>::GetInstance()
