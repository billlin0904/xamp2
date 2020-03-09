//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QFileSystemWatcher>

#include <base/metadata.h>

class FileSystemWatcher : public QObject {
	Q_OBJECT
public:
	explicit FileSystemWatcher(QObject* parent = nullptr);

	void addPath(const QString& path);

private slots:
	void onFileChanged(const QString& file);

	void onDirectoryChanged(const QString& path);

	void onReadCompleted(const std::vector<xamp::base::Metadata>& medata);

private:
	QFileSystemWatcher watcher_;
	QMap<QString, QStringList> file_map_;
};

