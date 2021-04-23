#include <stack>
#include <set>

#include <base/stl.h>
#include <base/windows_handle.h>
#include <base/fastmutex.h>
#include <base/exception.h>
#include <base/logger.h>
#include <widget/directorywatcher.h>

#ifdef XAMP_OS_WIN
using namespace xamp::base;

static Path getPath(WCHAR const* path, const DWORD size_in_bytes) {
    return { path, path + size_in_bytes / sizeof(WCHAR) };
}

static HANDLE attachHandle(HANDLE iocp_handle, HANDLE dir_handle) {
    return ::CreateIoCompletionPort(dir_handle, iocp_handle,
        reinterpret_cast<ULONG_PTR>(dir_handle),
        1);
}

static HANDLE createDirHandle(std::wstring const& path) {
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
    WinHandle handle;
    std::array<uint8_t, kChangesBufferSize> buf;
    Path full_path;
};

class DirectoryWatcher::WatcherWorkerImpl {
public:
    explicit WatcherWorkerImpl(DirectoryWatcher* watcher)
        : watcher_(watcher) {
        iocp_handle_.reset(attachHandle(nullptr, INVALID_HANDLE_VALUE));
    }

    void addPath(std::wstring const& path) {
        std::lock_guard<FastMutex> guard{ mutex_ };
        if (watch_file_handles_.find(path) != watch_file_handles_.end()) {
            return;
        }
        auto dir_handle(createDirHandle(path));
        attachHandle(iocp_handle_.get(), dir_handle);
        startMonitor(dir_handle, path);
    }

    void removePath(std::wstring const& path) {
        std::lock_guard<FastMutex> guard{ mutex_ };
        watch_file_handles_.erase(path);
    }

	void monitorChange(OverlappedEx * ov) {
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

    void startMonitor(HANDLE dir_handle, std::wstring const& path) {
        auto ov = MakeAlign<OverlappedEx>();
        ov->handle.reset(dir_handle);
        ov->buf.fill(0);
        ov->full_path = path;

        auto ret = ::ReadDirectoryChangesW(ov->handle.get(),
            ov->buf.data(),
            kChangesBufferSize,
            FALSE,
            kNotifyFilter,
            nullptr,
            ov.get(),
            nullptr
        );

        if (!ret) {
            throw PlatformSpecException();
        }
        watch_file_handles_[path] = std::move(ov);
    }

    void run() {
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
                requireUpdate(reinterpret_cast<OverlappedEx*>(ol));
                notifyChanges();
            }
            else if (key == kStopMonitorKey) {
                break;
            }
        }

        XAMP_LOG_DEBUG("WatcherWorker thread is shutdown!");
    }

    void notifyChanges() {
        for (auto& entry : change_entries_) {
            switch (entry.action) {
            case DirectoryAction::kAdd:
                watcher_->fileChanged(QString::fromStdWString(entry.new_path.wstring()));
                break;
            case DirectoryAction::kRemove:
                watcher_->fileChanged(QString::fromStdWString(entry.old_path.wstring()));
                break;
            case DirectoryAction::kRename:
                watcher_->fileChanged(QString::fromStdWString(entry.old_path.wstring()));                
                break;
            }        	
        }
        monitorChange(watch_file_handles_.begin()->second.get());
    }

    void shutdown() {
        ::PostQueuedCompletionStatus(iocp_handle_.get(),
            0,
            kStopMonitorKey,
            nullptr);
    }

    void requireUpdate(OverlappedEx const* ov) {
        std::stack<Path> rename_old_names;
        std::stack<Path> rename_new_names;

        const auto* buffer = reinterpret_cast<const uint8_t*>(ov->buf.data());
        const FILE_NOTIFY_INFORMATION* notifies = nullptr;
        change_entries_.clear();

        do {
            notifies = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer);
            auto raw_path = getPath(notifies->FileName, notifies->FileNameLength);

            switch (notifies->Action) {
            case FILE_ACTION_ADDED:
                change_entries_.push_back(
                    { DirectoryAction::kAdd, Path(), raw_path });
                break;
            case FILE_ACTION_REMOVED:
                change_entries_.push_back(
                    { DirectoryAction::kRemove, raw_path, Path() });
                break;
            case FILE_ACTION_MODIFIED:
                change_entries_.push_back(
                    { DirectoryAction::kModify, Path(), raw_path });
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                if (rename_new_names.empty()) {
                    rename_old_names.push(raw_path);
                }
                else {
                    change_entries_.push_back(
                        { DirectoryAction::kRename, raw_path, rename_old_names.top() });
                    rename_new_names.pop();
                }
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                if (rename_old_names.empty()) {
                    rename_new_names.push(raw_path);
                }
                else {
                    change_entries_.push_back(
                        { DirectoryAction::kRename, rename_old_names.top(),raw_path });
                    rename_old_names.pop();
                }
                break;
            }

            buffer += notifies->NextEntryOffset;

        } while (notifies->NextEntryOffset != 0);
    }

    WinHandle iocp_handle_;
    std::vector<DirectoryChangeEntry> change_entries_;
    HashMap<std::wstring, AlignPtr<OverlappedEx>> watch_file_handles_;
    FastMutex mutex_;
    DirectoryWatcher* watcher_;
};

DirectoryWatcher::DirectoryWatcher(QObject* parent)
	: QThread(parent)
	, impl_(MakeAlign<WatcherWorkerImpl>(this)) {
}

void DirectoryWatcher::run() {
    impl_->run();
}

void DirectoryWatcher::shutdown() {
    impl_->shutdown();
}

DirectoryWatcher::~DirectoryWatcher() {
}

void DirectoryWatcher::addPath(const QString& file) {
    impl_->addPath(file.toStdWString());
}

void DirectoryWatcher::removePath(const QString& file) {
}
#endif