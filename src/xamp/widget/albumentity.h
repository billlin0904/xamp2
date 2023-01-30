//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QModelIndex>

QVariant GetIndexValue(const QModelIndex& index, const QModelIndex& src, int i);
QVariant GetIndexValue(const QModelIndex& index, int i);

