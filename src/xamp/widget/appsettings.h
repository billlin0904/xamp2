//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

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

	void loadIniFile(const QString& file_name);

	template <typename T>
	void setValue(const QString& key, T value) {		
		settings_->setValue(key, value);
	}

	template <typename T>
	void setDefaultValue(const QString& key, T value) {
		default_settings_[key] = value;
	}

	xamp::base::ID getIDValue(const QString& key) const;

	QSize getSizeValue(const QString& width_key, const QString& height_key) const;

	QVariant getValue(const QString& key) const;

	int32_t getAsInt(const QString& key) const;
	
protected:
	AppSettings() = default;

private:
	static xamp::base::AlignPtr<QSettings> settings_;
	static QMap<QString, QVariant> default_settings_;
};
