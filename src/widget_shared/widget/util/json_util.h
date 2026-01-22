//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>

namespace json_util {

inline bool deserialize(const QString& str, QJsonDocument& doc) {
	QJsonParseError error;
	doc = QJsonDocument::fromJson(str.toUtf8(), &error);
	return error.error == QJsonParseError::NoError;
}

inline bool deserializeFile(const QString& file_path, QJsonDocument& doc) {
	QFile file_(file_path);
	if (!file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	auto file_data = file_.readAll();
	file_.close();

	QJsonParseError error;
	doc = QJsonDocument::fromJson(file_data, &error);
	return error.error == QJsonParseError::NoError;
}

inline QString serialize(const QJsonObject &object) {
	return QString::fromUtf8((QJsonDocument(object).toJson()));
}

inline QString serialize(const QVariantMap& object) {
	return QString::fromUtf8((QJsonDocument(QJsonObject::fromVariantMap(object)).toJson()));
}

inline QString serialize(const QVariantHash& object) {
	return QString::fromUtf8((QJsonDocument(QJsonObject::fromVariantHash(object)).toJson()));
}

}
