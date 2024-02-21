#include <widget/jsonsettings.h>

#include <QJsonDocument>
#include <QIODevice>

#include <widget/util/str_utilts.h>

namespace {
	bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map) {
		QJsonParseError error;
		map = QJsonDocument::fromJson(device.readAll(), &error).toVariant().toMap();
		return error.error == QJsonParseError::NoError;
	}

	bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map) {
		bool ret = false;
		const auto json_document = QJsonDocument::fromVariant(QVariant::fromValue(map));
		if (device.write(json_document.toJson()) != -1) {
			ret = true;
		}
		return ret;
	}
}

void JsonSettings::loadJsonFile(const QString& file_name) {
	const auto json_format =
		QSettings::registerFormat(qTEXT("json"), readJsonFile, writeJsonFile);
	settings_.reset(new QSettings(file_name, json_format));
}

QStringList JsonSettings::keys() const {
	return settings_->allKeys();
}

void JsonSettings::remove(const QString& key) const {
	settings_->remove(key);
}

void JsonSettings::save() const {
	if (!settings_) {
		return;
	}
	settings_->sync();
}

int32_t JsonSettings::valueAsInt(const QString& key) const {
	return valueAs(key).toInt();
}

QMap<QString, QVariant> JsonSettings::valueAsMap(QString const& key) {
	return QVariant::fromValue(valueAs(key)).toMap();
}

QVariant JsonSettings::valueAs(const QString& key) const {
	if (key.isEmpty()) {
		return{};
	}
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	return settings_->value(key);
}
