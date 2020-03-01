//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QFileSystemWatcher>

#include <widget/metadataextractadapter.h>

class FileSystemWatcher : public QObject {
	Q_OBJECT
public:
	explicit FileSystemWatcher(QObject* parent = nullptr);

	~FileSystemWatcher() override;

	void addPath(const QString& path);

private slots:
	void onFileChanged(const QString& file);

	void onDirectoryChanged(const QString& path);

private:
	QFileSystemWatcher watcher_;
	MetadataExtractAdapter adapter_;
	QMap<QString, QStringList> file_map_;
};

