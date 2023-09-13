#include <widget/jsonsettings.h>

#include <QJsonDocument>
#include <QIODevice>

#include <widget/str_utilts.h>

namespace {
	bool ReadJsonFile(QIODevice& device, QSettings::SettingsMap& map) {
		QJsonParseError error;
		map = QJsonDocument::fromJson(device.readAll(), &error).toVariant().toMap();
		return error.error == QJsonParseError::NoError;
	}

	bool WriteJsonFile(QIODevice& device, const QSettings::SettingsMap& map) {
		bool ret = false;
		const auto json_document = QJsonDocument::fromVariant(QVariant::fromValue(map));
		if (device.write(json_document.toJson()) != -1) {
			ret = true;
		}
		return ret;
	}
}

QScopedPointer<QSettings> JsonSettings::settings_;
QMap<QString, QVariant> JsonSettings::default_settings_;

void JsonSettings::LoadJsonFile(const QString& file_name) {
	const auto json_format =
		QSettings::registerFormat(qTEXT("json"), ReadJsonFile, WriteJsonFile);
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

int32_t JsonSettings::GetAsInt(const QString& key) {
	return GetValue(key).toInt();
}

QMap<QString, QVariant> JsonSettings::ValueAsMap(QString const& key) {
	return QVariant::fromValue(GetValue(key)).toMap();
}

QVariant JsonSettings::GetValue(const QString& key) {
	if (key.isEmpty()) {
		return{};
	}
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	return settings_->value(key);
}