//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFileSystemModel>
#include <widget/widget_shared_global.h>

class FileSystemModel : public QFileSystemModel {
public:
	explicit FileSystemModel(QObject* parent = nullptr);

	QVariant data(const QModelIndex& index, int role) const override;
};