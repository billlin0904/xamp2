#include "appsettings.h"

const QLatin1String APP_SETTING_LANG{ "AppSettings/lang" };
const QLatin1String APP_SETTING_DEVICE_TYPE{ "AppSettings/DeviceType" };
const QLatin1String APP_SETTING_DEVICE_ID{ "AppSettings/DeviceId" };
const QLatin1String APP_SETTING_WIDTH{ "AppSettings/width" };
const QLatin1String APP_SETTING_HEIGHT{ "AppSettings/height" };
const QLatin1String APP_SETTING_VOLUME{ "AppSettings/volume" };
const QLatin1String APP_SETTING_ORDER{ "AppSettings/order" };
const QLatin1String APP_SETTING_NIGHT_MODE{ "AppSettings/nightMode" };
const QLatin1String APP_SETTING_BACKGROUND_COLOR{ "AppSettings/theme/backgroundColor" };
const QLatin1String APP_SETTING_ENABLE_BLUR{ "AppSettings/theme/enableBlur" };
const QLatin1String APP_SETTING_MUSIC_FILE_PATH{ "AppSettings/musicFilePath" };

const QLatin1String APP_SETTING_RESAMPLER_ENABLE{ "AppSettings/soxr/enable" };
const QLatin1String APP_SETTING_SOXR_RESAMPLE_SAMPLRATE{ "AppSettings/soxr/resampleSampleRate" };
const QLatin1String APP_SETTING_SOXR_ENABLE_STEEP_FILTER{ "AppSettings/soxr/enableSteepFilter" };
const QLatin1String APP_SETTING_SOXR_QUALITY{ "AppSettings/soxr/quality" };
const QLatin1String APP_SETTING_SOXR_PHASE{ "AppSettings/soxr/phase" };
const QLatin1String APP_SETTING_SOXR_PASS_BAND{ "AppSettings/soxr/passBand" };

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));
}

xamp::base::ID AppSettings::getID(const QString& key) {
	auto str = getValue(key).toString();
	if (str.isEmpty()) {
		return xamp::base::ID::INVALID_ID;
	}
	return xamp::base::ID::FromString(str.toStdString());
}

QSize AppSettings::getSizeValue(const QString& width_key,
	const QString& height_key) {
	return QSize{
		getAsInt(width_key),
		getAsInt(height_key),
	};
}

QVariant AppSettings::getValue(const QString& key) {
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	return settings_->value(key);
}

int32_t AppSettings::getAsInt(const QString& key) {
	return getValue(key).toInt();
}