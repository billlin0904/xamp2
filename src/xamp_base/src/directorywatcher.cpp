#include <stack>
#include <set>

#include <base/stl.h>
#include <base/windows_handle.h>
#include <base/fastmutex.h>
#include <base/exception.h>
#include <base/logger.h>
#include <base/directorywatcher.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
static Path GetPath(WCHAR const* path, const DWORD size_in_bytes) {
    return { path, path + size_in_bytes / sizeof(WCHAR) };
}

static HANDLE AttachHandle(HANDLE iocp_handle, HANDLE dir_handle) {
    return ::CreateIoCompletionPort(dir_handle, iocp_handle,
        reinterpret_cast<ULONG_PTR>(dir_handle),
        1);
}

static HANDLE CreateDirHandle(std::wstring const& path) {
    return ::CreateFileW(path.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );
}

static constexpr std::size_t kChangesBufferSize = 65536;
static constexpr ULONG_PTR kStopMonitorKey = -1;
static constexpr DWORD kNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_LAST_WRITE |
    FILE_NOTIFY_CHANGE_DIR_NAME |
    FILE_NOTIFY_CHANGE_SIZE;

struct OverlappedEx : OVERLAPPED {
    Path full_path;
    WinHandle handle;
    std::array<uint8_t, kChangesBufferSize> buf;
};

class DirectoryWatcher::WatcherWorkerImpl {
public:
    explicit WatcherWorkerImpl(std::weak_ptr<IFileChangedCallback> callback)
        : callback_(callback) {
        iocp_handle_.reset(AttachHandle(nullptr, INVALID_HANDLE_VALUE));
    }

    void AddPath(std::wstring const& path) {
        std::lock_guard guard{ lock_ };
        if (watch_file_handles_.find(path) != watch_file_handles_.end()) {
            return;
        }
        WinHandle dir_handle(CreateDirHandle(path));
        auto ov = MakeAlign<OverlappedEx>();
        if (AttachHandle(iocp_handle_.get(), dir_handle.get()) != iocp_handle_.get()) {
            throw PlatformSpecException();
        }
        ov->handle = std::move(dir_handle);
        ov->buf.fill(0);
        ov->full_path = path;
        ReadDirAsync(ov.get());
        watch_file_handles_[path] = std::move(ov);
    }

    void RemovePath(std::wstring const& path) {
        std::lock_guard guard{ lock_ };
        watch_file_handles_.erase(path);
    }

    void ReadDirAsync(OverlappedEx* ov) {
        auto ret = ::ReadDirectoryChangesW(ov->handle.get(),
            ov->buf.data(),
            kChangesBufferSize,
            FALSE,
            kNotifyFilter,
            nullptr,
            ov,
            nullptr
        );

        if (!ret) {
            throw PlatformSpecException();
        }
    }

    void Run() {
        DWORD bytes_read = 0;
        ULONG_PTR key = 0;
        OVERLAPPED* ol = nullptr;

        for (;;) {
            auto ret = ::GetQueuedCompletionStatus(iocp_handle_.get(),
                &bytes_read,
                &key,
                &ol,
                INFINITE);
            if (ret && bytes_read > 0) {
                RequireUpdate(reinterpret_cast<OverlappedEx*>(ol));
                NotifyChanges();
            }
            else if (key == kStopMonitorKey) {
                break;
            }
        }

        XAMP_LOG_DEBUG("WatcherWorker thread is shutdown!");
    }

    void NotifyChanges() {
        auto callback = callback_.lock();
        if (!callback) {
            return;
        }
        for (const auto& entry : change_entries_) {
            switch (entry.action) {
            case FileChangeAction::kAdd:
                callback->OnFileChanged(entry.new_path.wstring());
                break;
            case FileChangeAction::kRemove:
                callback->OnFileChanged(entry.old_path.wstring());
                break;
            case FileChangeAction::kRename:
                callback->OnFileChanged(entry.old_path.wstring());
                break;
            case FileChangeAction::kModify:
                callback->OnFileChanged(entry.new_path.wstring());
                break;
            }
        }
        std::lock_guard guard{ lock_ };
        ReadDirAsync(watch_file_handles_.begin()->second.get());
    }

    void Shutdown() {
        ::PostQueuedCompletionStatus(iocp_handle_.get(),
            0,
            kStopMonitorKey,
            nullptr);
    }

    void RequireUpdate(OverlappedEx const* ov) {
        std::stack<Path> rename_old_names;
        std::stack<Path> rename_new_names;

        const auto* buffer = ov->buf.data();
        const FILE_NOTIFY_INFORMATION* notifies = nullptr;
        change_entries_.clear();

        do {
            notifies = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer);
            auto raw_path = GetPath(notifies->FileName, notifies->FileNameLength);

            switch (notifies->Action) {
            case FILE_ACTION_ADDED:
                change_entries_.push_back(
                    { FileChangeAction::kAdd, Path(), raw_path });
                break;
            case FILE_ACTION_REMOVED:
                change_entries_.push_back(
                    { FileChangeAction::kRemove, raw_path, Path() });
                break;
            case FILE_ACTION_MODIFIED:
                change_entries_.push_back(
                    { FileChangeAction::kModify, Path(), raw_path });
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                if (rename_new_names.empty()) {
                    rename_old_names.push(raw_path);
                }
                else {
                    change_entries_.push_back(
                        { FileChangeAction::kRename, raw_path, rename_old_names.top() });
                    rename_new_names.pop();
                }
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                if (rename_old_names.empty()) {
                    rename_new_names.push(raw_path);
                }
                else {
                    change_entries_.push_back(
                        { FileChangeAction::kRename, rename_old_names.top(),raw_path });
                    rename_old_names.pop();
                }
                break;
            }

            buffer += notifies->NextEntryOffset;

        } while (notifies->NextEntryOffset != 0);
    }

    FastMutex lock_;
    WinHandle iocp_handle_;
    std::thread thread_;
    std::vector<DirectoryChangeEntry> change_entries_;
    HashMap<std::wstring, AlignPtr<OverlappedEx>> watch_file_handles_;
    std::weak_ptr<IFileChangedCallback> callback_;
};
#else
class DirectoryWatcher::WatcherWorkerImpl {
public:
    explicit WatcherWorkerImpl(std::weak_ptr<FileChangedCallback> callback) {
    }
};
#endif


XAMP_PIMPL_IMPL(DirectoryWatcher)

DirectoryWatcher::DirectoryWatcher(std::weak_ptr<IFileChangedCallback> callback)
	: impl_(MakeAlign<WatcherWorkerImpl>(callback)) {
}

void DirectoryWatcher::AddPath(std::wstring const& path) {	
}

void DirectoryWatcher::RemovePath(std::wstring const& path) {	
}

void DirectoryWatcher::Shutdown() {
}

}
