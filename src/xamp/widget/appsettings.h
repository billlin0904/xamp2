//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string_view>

#include <base/align_ptr.h>
#include <base/id.h>

#include <QSize>
#include <QSettings>

extern const QLatin1String APP_SETTING_DEVICE_TYPE;
extern const QLatin1String APP_SETTING_DEVICE_ID;
extern const QLatin1String APP_SETTING_WIDTH;
extern const QLatin1String APP_SETTING_HEIGHT;
extern const QLatin1String APP_SETTING_VOLUME;
extern const QLatin1String APP_SETTING_ORDER;
extern const QLatin1String APP_SETTING_NIGHT_MODE;

class AppSettings {
public:
	static AppSettings& settings() {
		static AppSettings instance;
		return instance;
	}

	void loadIniFile(const QString &file_name) {
		settings_ = xamp::base::MakeAlign<QSettings>(file_name, QSettings::IniFormat);
	}

	template <typename T>
	void setValue(const QString& key, T value) {		
		settings_->setValue(key, value);
	}

	xamp::base::ID getIDValue(const QString& key) const {
		auto str = getValue(key).toString();
		if (str.isEmpty()) {
			return xamp::base::ID::INVALID_ID;
		}
		return xamp::base::ID::FromString(str.toStdString());
	}

	QSize getSizeValue(const QString& width_key,
		const QString& height_key) const {
		return QSize{
			getValue(width_key).toInt(),
			getValue(height_key).toInt(),
		};
	}

	QVariant getValue(const QString& key) const {
		if (!settings_->contains(key)) {
			return default_settings_.value(key);
		}
		return settings_->value(key);
	}

    int32_t getAsInt(const QString& key) const {
        return getValue(key).toInt();
    }

	template <typename T>
	void setDefaultValue(const QString& key, T value) {
		default_settings_[key] = value;
	}
protected:
	AppSettings() = default;

private:
	static xamp::base::AlignPtr<QSettings> settings_;
	static QMap<QString, QVariant> default_settings_;
};
