//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

namespace json_util {

inline bool deserialize(const QString& str, QVariantMap &value) {
	QJsonParseError error;
	value = QJsonDocument::fromJson(str.toUtf8(), &error).toVariant().toMap();
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
