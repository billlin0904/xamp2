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

static const QLatin1String APP_SETTING_DEVICE_TYPE{ "AppSettings/DeviceType" };
static const QLatin1String APP_SETTING_DEVICE_ID{ "AppSettings/DeviceId" };
static const QLatin1String APP_SETTING_WIDTH{ "AppSettings/width" };
static const QLatin1String APP_SETTING_HEIGHT{ "AppSettings/height" };
static const QLatin1String APP_SETTING_VOLUME{ "AppSettings/volume" };
static const QLatin1String APP_SETTING_ORDER{ "AppSettings/order" };
static const QLatin1String APP_SETTING_NIGHT_MODE{ "AppSettings/nightMode" };

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
	void setSettingValue(const QString& key, T value) {		
		settings_->setValue(key, value);
	}

	xamp::base::ID getSettingID(const QString& key) const {
		auto str = getSettingValue(key).toString();
		if (str.isEmpty()) {
			return xamp::base::ID::INVALID_ID;
		}
		return xamp::base::ID::FromString(str.toStdString());
	}

	QSize getSettingSize(const QString& width_key,
		const QString& height_key) const {
		return QSize{
			getSettingValue(width_key).toInt(),
			getSettingValue(height_key).toInt(),
		};
	}

	QVariant getSettingValue(const QString& key) const {
		if (!settings_->contains(key)) {
			return default_settings_.value(key);
		}
		return settings_->value(key);
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
