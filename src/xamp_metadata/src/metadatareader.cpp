#include <base/base.h>
#include <base/logger.h>

#ifdef XAMP_OS_WIN
#include <winioctl.h>
#include <base/exception.h>
#include <base/windows_handle.h>
#include <base/threadpool.h>
#include <base/platform_thread.h>
#endif

#include <metadata/metadataextractadapter.h>
#include <metadata/metadatareader.h>

namespace xamp::metadata {

#ifdef XAMP_OS_WIN
static FILE_ID_DESCRIPTOR GetFileIdDescriptor(const DWORDLONG file_id) {
    FILE_ID_DESCRIPTOR file_descriptor{0};
    file_descriptor.Type = FileIdType;
    file_descriptor.FileId.QuadPart = file_id;
    file_descriptor.dwSize = sizeof(file_descriptor);
    return file_descriptor;
}

void IndexVolum() {
    if (!EnablePrivilege("SeBackupPrivilege", true)) {
        return;
    }
	
    if (!EnablePrivilege("SeRestorePrivilege", true)) {
        return;
    }
	
    FileHandle device(::CreateFileW(
        L"\\??\\C:",
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr));
	if (!device) {
        XAMP_LOG_DEBUG("CreateFileW return failure! {}",
            GetPlatformErrorMessage(::GetLastError()));
        return;
	}

    USN_JOURNAL_DATA ujd = { 0 };
    DWORD cb = 0;
    std::vector<BYTE> data(sizeof(DWORDLONG) + 0x10000);

    if (!::DeviceIoControl(device.get(),
        FSCTL_QUERY_USN_JOURNAL,
        nullptr,
        0,
        &ujd,
        sizeof(USN_JOURNAL_DATA),
        &cb, 
        nullptr)) {
        XAMP_LOG_DEBUG("DeviceIoControl return failure! {}: {}",
            ::GetLastError(),
            GetPlatformErrorMessage(::GetLastError()));
        return;
    }

    MFT_ENUM_DATA med = { 0 };
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = ujd.NextUsn;

    ThreadPool dispatch;

	while (true) {
        if (!::DeviceIoControl(device.get(),
            FSCTL_ENUM_USN_DATA,
            &med,
            sizeof(med),
            data.data(),
            data.size(),
            &cb,
            nullptr)) {
            XAMP_LOG_DEBUG("DeviceIoControl return failure! {}",
                GetPlatformErrorMessage(::GetLastError()));
            break;
        }

        auto* record = reinterpret_cast<PUSN_RECORD>(&data[sizeof(USN)]);
        while (reinterpret_cast<PBYTE>(record) < (data.data() + cb)) {
            std::wstring sz(reinterpret_cast<LPCWSTR>(reinterpret_cast<PBYTE>(record) + record->FileNameOffset),
                record->FileNameLength / sizeof(WCHAR));
            record = reinterpret_cast<PUSN_RECORD>(reinterpret_cast<PBYTE>(record) + record->RecordLength);
            auto fd = GetFileIdDescriptor(record->FileReferenceNumber);
            auto f = dispatch.StartNew([&]() {
                wchar_t file_path[MAX_PATH];
                const FileHandle open_file(::OpenFileById(device.get(), 
                                                          const_cast<LPFILE_ID_DESCRIPTOR>(&fd),
                                                          0,
                                                          0,
                                                          nullptr,
                                                          0));
            	if (!open_file) {
            		return;
            	}
                ::GetFinalPathNameByHandleW(open_file.get(),
                    file_path,
                    MAX_PATH,
                    0);
                });
            f.get();
        }
	}
}	
#endif
	
void FromPath(const Path& path, MetadataExtractAdapter* adapter, MetadataReader *reader) {
    using namespace std::filesystem;
    const auto options = (
        directory_options::follow_directory_symlink |
        directory_options::skip_permission_denied
        );
	
    if (is_directory(path)) {
        Path root_path;
        for (const auto& file_or_dir : RecursiveDirectoryIterator(path, options)) {
            if (adapter->IsCancel()) {
                return;
            }

            const auto & current_path = file_or_dir.path();
            if (root_path.empty()) {
                root_path = current_path;
            }

            auto parent_path = root_path.parent_path();
            auto cur_path = current_path.parent_path();
            if (parent_path != cur_path) {
                adapter->OnWalkNext();
                root_path = current_path;
            }

            if (!is_directory(current_path)) {
                if (reader->IsSupported(current_path)) {
                    adapter->OnWalk(path, reader->Extract(current_path));
                }
            }
        }
        adapter->OnWalkNext();
    }
    else {
        if (reader->IsSupported(path)) {
            adapter->OnWalk(path, reader->Extract(path));
            adapter->OnWalkNext();
        }
    }
}

}
