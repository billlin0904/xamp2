#pragma once

#include <memory>
#include <vector>

#include <QDir>
#include <QFileIconProvider>
#include <QDateTime>

#include "str_utilts.h"

class FileSystemEntity 
	: public std::enable_shared_from_this<FileSystemEntity> {
public:
	explicit FileSystemEntity(const QFileInfo& file_info);

	void setParent(std::shared_ptr<FileSystemEntity> parent);

	QString absoluteFilePath() const;

	std::shared_ptr<FileSystemEntity> parent();

	void addChild(std::shared_ptr<FileSystemEntity> child);

	QString fileName() const;

	std::shared_ptr<FileSystemEntity> matchPath(const QStringList& path, int32_t startIndex = 0);

	int childCount() const;

	QFileInfo fileInfo() const;

	FileSystemEntity* childAt(int position);

	int childNumber() const;

private:	
	QFileInfo file_info_;
	std::weak_ptr<FileSystemEntity> parent_;
	QString absolute_file_path_;
	std::vector<std::shared_ptr<FileSystemEntity>> children_;
};
