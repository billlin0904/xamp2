#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>
#include <QSize>

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
	auto folder_path = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
	if (folder_path.isEmpty()) {
		return Qt::EmptyString;
	}
	return folder_path[0];
}

void AppSettings::setOrDefaultConfig() {
    loadIniFile(Q_UTF8("xamp.ini"));
    setDefaultValue(kAppSettingDeviceType, Qt::EmptyString);
    setDefaultValue(kAppSettingDeviceId, Qt::EmptyString);
    setDefaultValue(kAppSettingWidth, 600);
    setDefaultValue(kAppSettingHeight, 500);
    setDefaultValue(kAppSettingVolume, 50);
    setDefaultValue(kAppSettingNightMode, false);
    setDefaultValue(kAppSettingOrder, static_cast<int32_t>(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE));
    setDefaultValue(kAppSettingBackgroundColor, QColor("#01121212"));
    setDefaultValue(kAppSettingEnableBlur, true);
	setDefaultValue(kAppSettingPreventSleep, true);
    setDefaultValue(kLyricsFontSize, 12);
    setDefaultValue(kAppSettingMinimizeToTrayAsk, true);
    setDefaultValue(kAppSettingMinimizeToTray, false);
}

Uuid AppSettings::getID(const QString& key) {
	auto str = getValue(key).toString();
	if (str.isEmpty()) {
		return Uuid::INVALID_ID;
	}
	return Uuid::FromString(str.toStdString());
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
