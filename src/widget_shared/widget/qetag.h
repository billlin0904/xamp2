//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>

namespace qetag {

QString getTagId(const QByteArray& buffer) noexcept;
QString getTagId(const QString& file_name) noexcept;

}

