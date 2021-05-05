//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QThread>
#include <QObject>

#include <base/align_ptr.h>
#include <metadata/metadata.h>

enum class FileChangeAction {
	kRename,
	kModify,
	kRemove,
	kAdd,
};

using xamp::metadata::Path;
using xamp::base::AlignPtr;

struct DirectoryChangeEntry {
	FileChangeAction action;
	Path old_path;
	Path new_path;
};

class DirectoryWatcher : public QThread {
	Q_OBJECT
public:
	explicit DirectoryWatcher(QObject* parent = nullptr);

	~DirectoryWatcher();

	void shutdown();

	void addPath(const QString& file);
	
	void removePath(const QString& file);
	
Q_SIGNALS:
	void fileChanged(const QString& path);

protected:
	void run() override;

private:
#ifdef XAMP_OS_WIN
	class WatcherWorkerImpl;
	AlignPtr<WatcherWorkerImpl> impl_;
#endif
};

