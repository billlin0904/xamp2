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
	const auto json_document = QJsonDocument::fromVariant(QVariant::fromValue(map));
	if (device.write(json_document.toJson()) != -1) {
		ret = true;
	}
	return ret;
}

QScopedPointer<QSettings> JsonSettings::settings_;
QMap<QString, QVariant> JsonSettings::default_settings_;

void JsonSettings::loadJsonFile(const QString& file_name) {
	const auto json_format =
		QSettings::registerFormat(Q_TEXT("json"), readJsonFile, writeJsonFile);
	settings_.reset(new QSettings(file_name, json_format));
}

QStringList JsonSettings::keys() {
	return settings_->allKeys();
}

void JsonSettings::remove(const QString& key) {
	settings_->remove(key);
}

void JsonSettings::save() {
	if (!settings_) {
		return;
	}
	settings_->sync();
}

int32_t JsonSettings::getAsInt(const QString& key) {
	return getValue(key).toInt();
}

QMap<QString, QVariant> JsonSettings::getValueAsMap(QString const& key) {
	return QVariant::fromValue(getValue(key)).toMap();
}

QVariant JsonSettings::getValue(const QString& key) {
	if (key.isEmpty()) {
		return{};
	}
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	return settings_->value(key);
}