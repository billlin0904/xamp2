//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QCryptographicHash>
#include <QString>

template <typename T>
void addDataToHash(QCryptographicHash& hash, const T& arg) {
    hash.addData(arg.toUtf8());
}

template <>
inline void addDataToHash(QCryptographicHash& hash, const QByteArray& arg) {
    hash.addData(arg);
}

template <typename... Args>
QString generateHash(const Args&... args) {
    QCryptographicHash hash{ QCryptographicHash::Md5 };
    (addDataToHash(hash, args), ...);

    return QString::fromUtf8(hash.result().toHex());
}

template <typename... Args>
QByteArray generateMd5Hash(const Args&... args) {
    QCryptographicHash hash{ QCryptographicHash::Md5 };
    (addDataToHash(hash, args), ...);
    return hash.result();
}