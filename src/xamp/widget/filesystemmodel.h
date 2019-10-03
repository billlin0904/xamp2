//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QAbstractItemModel>
#include <QFileIconProvider>

#include "filesystementity.h"

class FileSystemModel : public QAbstractItemModel {
	Q_OBJECT
public:
	enum Column {
		COLUMN_NAME,
		COLUMN_SIZE,
		COLUMN_TYPE,
		COLUMN_DATE,
		_MAX_COLUMN_
	};

	explicit FileSystemModel(QObject* parent = nullptr);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	int columnCount(const QModelIndex&) const override;

	int rowCount(const QModelIndex& parent) const override;

	QVariant data(const QModelIndex& index, int role) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent) const override;

	QModelIndex index(const QString& path, int column) const;

	bool isDir(const QModelIndex& index) const;

	QString absolutePath(const QModelIndex& index) const;

	QString currentPath() const;

	QModelIndex setCurrentPath(const QString& path);
private:
	void populateItem(FileSystemEntity* item);

	FileSystemEntity* getItem(const QModelIndex& index) const;

	QString current_path_;
	QStringList headers_;
	QFileIconProvider icon_provider_;
	std::shared_ptr<FileSystemEntity> root_;
};

