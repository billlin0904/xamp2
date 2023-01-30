//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QByteArray>

namespace QEtag {

QString GetTagId(const QByteArray& buffer) noexcept;
QString GetTagId(const QString& file_name) noexcept;

}

