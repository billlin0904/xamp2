//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>

namespace xamp::base {

enum class FileChangeAction {
	kRename,
	kModify,
	kRemove,
	kAdd,
};

struct DirectoryChangeEntry {
	FileChangeAction action;
	Path old_path;
	Path new_path;
};

class FileChangedCallback {
public:
	virtual ~FileChangedCallback() = default;
	virtual void OnFileChanged(std::wstring const& path) = 0;
protected:
	FileChangedCallback() = default;
};

class DirectoryWatcher {
public:
	explicit DirectoryWatcher(std::weak_ptr<FileChangedCallback> callback);

	XAMP_PIMPL(DirectoryWatcher)

	void AddPath(std::wstring const& path);

	void RemovePath(std::wstring const& path);

	void Shutdown();
private:
	class WatcherWorkerImpl;
	AlignPtr<WatcherWorkerImpl> impl_;
};

}