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
	DirectoryWatcher();

	virtual ~DirectoryWatcher() override;

	void shutdown();

Q_SIGNALS:
	void fileChanged(const QString& path);

public slots:
	void addPath(const QString& file);

	void removePath(const QString& file);

protected:
	void run() override;
	
private:
#ifdef XAMP_OS_WIN
	class WatcherWorkerImpl;
	AlignPtr<WatcherWorkerImpl> impl_;
#endif
};

