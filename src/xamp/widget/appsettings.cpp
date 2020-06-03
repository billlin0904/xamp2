#include <QStandardPaths>
#include <widget/playerorder.h>
#include <widget/appsettings.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));
}

void AppSettings::save() {
	if (!settings_) {
		return;
	}
    settings_->sync();
}

QString AppSettings::getMyMusicFolderPath() {
	return QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0];
}

void AppSettings::setOrDefaultConfig() {
    loadIniFile(Q_UTF8("xamp.ini"));
    setDefaultValue(kAppSettingDeviceType, QEmptyString);
    setDefaultValue(kAppSettingDeviceId, QEmptyString);
    setDefaultValue(kAppSettingWidth, 600);
    setDefaultValue(kAppSettingHeight, 500);
    setDefaultValue(kAppSettingVolume, 50);
    setDefaultValue(kAppSettingNightMode, false);
    setDefaultValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
    setDefaultValue(kAppSettingBackgroundColor, QColor("#01121212"));
    setDefaultValue(kAppSettingEnableBlur, true);
	setDefaultValue(kAppSettingPreventSleep, true);
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

void AppSettings::loadLanguage(const QString& lang) {
	manager_.loadLanguage(lang);
}
