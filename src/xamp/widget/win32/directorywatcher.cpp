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

static constexpr std::size_t kChangesBufferSize = 64 * 1024;
static constexpr ULONG_PTR kStopMonitorKey = -1;

struct OverlappedEx : OVERLAPPED {
    OverlappedEx(HANDLE handle, Path const & full_path)
	    : handle(handle)
		, full_path(full_path) {	    
    }
    WinHandle handle;
    uint8_t buf[kChangesBufferSize];
    Path full_path;
};

class DirectoryWatcher::WatcherWorkerImpl {
public:
    WatcherWorkerImpl() {
        iocp_handle_.reset(attachHandle(nullptr, INVALID_HANDLE_VALUE));
    }

	void addPath(std::wstring const &path) {
        std::lock_guard<FastMutex> guard{ mutex_ };        
        const auto dir_handle = createDirHandle(path);        
        attachHandle(iocp_handle_.get(), dir_handle);
        startMonitor(dir_handle, path);
	}

    void removePath(std::wstring const& path) {
        std::lock_guard<FastMutex> guard{ mutex_ };
        watch_files_.erase(path);
        watch_file_handles_.erase(path);
    }
	
	void startMonitor(HANDLE dir_handle, std::wstring const& path) {
        auto ov = MakeAlign<OverlappedEx>(dir_handle, path);
    	
        ::ReadDirectoryChangesW(dir_handle,
            ov->buf,
            kChangesBufferSize,
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME
            | FILE_NOTIFY_CHANGE_DIR_NAME
            | FILE_NOTIFY_CHANGE_SIZE
            | FILE_NOTIFY_CHANGE_LAST_WRITE,
            nullptr,
            ov.get(),
            nullptr
        );

        watch_file_handles_[path] = std::move(ov);
        watch_files_.insert(path);
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
        	} else if (key == kStopMonitorKey) {
                break;
        	}
    	}
    	
        XAMP_LOG_DEBUG("WatcherWorker thread is shutdown!");
    }

    void shutdown() {
        ::PostQueuedCompletionStatus(iocp_handle_.get(),
            0,
            kStopMonitorKey,
            nullptr);
    }
	
	void requireUpdate(OverlappedEx const *ov) {
        std::stack<Path> rename_old_names;
        std::stack<Path> rename_new_names;
		
        const auto* buffer = reinterpret_cast<const uint8_t*>(ov->buf);
        const auto* notifies = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer);

		do {            
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
                if (rename_old_names.empty()) {
                    rename_old_names.push(raw_path);
                } else {
                    change_entries_.push_back(
                        { DirectoryAction::kRename, raw_path, rename_old_names.top() });
                }
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                if (rename_old_names.empty()) {
                    rename_old_names.push(raw_path);
                } else {
                    change_entries_.push_back(
                        { DirectoryAction::kRename, rename_old_names.top(),raw_path });
                }
                break;
            }

            buffer += notifies->NextEntryOffset;
			
        } while (notifies->NextEntryOffset != 0);
	}

    WinHandle iocp_handle_;
    std::vector<DirectoryChangeEntry> change_entries_;
    std::set<Path> watch_files_;
    HashMap<std::wstring, AlignPtr<OverlappedEx>> watch_file_handles_;
    FastMutex mutex_;
};

DirectoryWatcher::DirectoryWatcher(QObject* parent)
	: QThread(parent)
	, impl_(MakeAlign<WatcherWorkerImpl>()) {
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