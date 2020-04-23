#include <QJsonDocument>
#include <QIODevice>

#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>

static bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map) {
	QJsonParseError error;
	map = QJsonDocument::fromJson(device.readAll(), &error).toVariant().toMap();
	return error.error == QJsonParseError::NoError;
}

static bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map) {
	bool ret = false;
	QJsonDocument jsonDocument = QJsonDocument::fromVariant(QVariant::fromValue(map));
	if (device.write(jsonDocument.toJson()) != -1) {
		ret = true;
	}
	return ret;
}

QScopedPointer<QSettings> JsonSettings::settings_;
QMap<QString, QVariant> JsonSettings::default_settings_;

void JsonSettings::loadJsonFile(const QString& file_name) {
	const QSettings::Format JsonFormat =
		QSettings::registerFormat(Q_UTF8("json"), readJsonFile, writeJsonFile);
	settings_.reset(new QSettings(file_name, JsonFormat));
}

QStringList JsonSettings::keys() {
	return settings_->allKeys();
}

void JsonSettings::remove(const QString& key) {
	settings_->remove(key);
}

void JsonSettings::save() {
	settings_->sync();
}

int32_t JsonSettings::getAsInt(const QString& key) {
	return getValue(key).toInt();
}

QVariant JsonSettings::getValue(const QString& key) {
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	return settings_->value(key);
}