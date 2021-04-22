//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QThread>
#include <QObject>

#include <base/align_ptr.h>
#include <metadata/metadata.h>

enum class DirectoryAction {
	kRename,
	kModify,
	kRemove,
	kAdd,
};

using xamp::metadata::Path;
using xamp::base::AlignPtr;

struct DirectoryChangeEntry {
	DirectoryAction action;
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
	void fileChanged(const QString& path, QPrivateSignal);

protected:
	void run() override;
	
private:
	class WatcherWorkerImpl;
	AlignPtr<WatcherWorkerImpl> impl_;
};

