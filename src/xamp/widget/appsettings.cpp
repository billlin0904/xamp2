#include "appsettings.h"

const QLatin1String APP_SETTING_DEVICE_TYPE{ "AppSettings/DeviceType" };
const QLatin1String APP_SETTING_DEVICE_ID{ "AppSettings/DeviceId" };
const QLatin1String APP_SETTING_WIDTH{ "AppSettings/width" };
const QLatin1String APP_SETTING_HEIGHT{ "AppSettings/height" };
const QLatin1String APP_SETTING_VOLUME{ "AppSettings/volume" };
const QLatin1String APP_SETTING_ORDER{ "AppSettings/order" };
const QLatin1String APP_SETTING_NIGHT_MODE{ "AppSettings/nightMode" };

xamp::base::AlignPtr<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
