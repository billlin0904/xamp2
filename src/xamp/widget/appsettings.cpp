#include "appsettings.h"

xamp::base::AlignPtr<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;