#include <base/base.h>
#include <base/logger.h>

#ifdef XAMP_OS_WIN
#include <winioctl.h>
#include <base/exception.h>
#include <base/str_utilts.h>
#include <base/windows_handle.h>
#include <base/platform_thread.h>
#endif

#include <metadata/metadataextractadapter.h>
#include <metadata/metadatareader.h>

namespace xamp::metadata {

#ifdef XAMP_OS_WIN

#define REPORT_ERROR(Msg) \
	XAMP_LOG_DEBUG(Msg##" return failure! error:{} {}", ::GetLastError(), GetPlatformErrorMessage(::GetLastError())) 

class SearchFileManager {
public:
    SearchFileManager();

    void ScanAudioFiles(const std::wstring &file_path = L"\\\\.\\c:");

private:
    static FILE_ID_DESCRIPTOR GetFileIdDescriptor(const DWORDLONG file_id) {
        FILE_ID_DESCRIPTOR file_descriptor{ 0 };
        file_descriptor.Type = FileIdType;
        file_descriptor.FileId.QuadPart = file_id;
        file_descriptor.dwSize = sizeof(file_descriptor);
        return file_descriptor;
    }
};

SearchFileManager::SearchFileManager() {
    if (!EnablePrivilege("SeBackupPrivilege", true)) {
        return;
    }

    if (!EnablePrivilege("SeRestorePrivilege", true)) {
        return;
    }
}
	
void SearchFileManager::ScanAudioFiles(const std::wstring& path) {
    FileHandle device(::CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr));
	if (!device) {
        REPORT_ERROR("CreateFileW");
        return;
	}

    USN_JOURNAL_DATA_V0 journal_data_v0 = { 0 };
    DWORD cb = 0;
    
    if (!::DeviceIoControl(device.get(),
        FSCTL_QUERY_USN_JOURNAL,
        nullptr,
        0,
        &journal_data_v0,
        sizeof(journal_data_v0),
        &cb, 
        nullptr)) {
        REPORT_ERROR("DeviceIoControl FSCTL_QUERY_USN_JOURNAL");
        return;
    }

    MFT_ENUM_DATA_V0 mft_enum_data_v0 = { 0 };
    mft_enum_data_v0.StartFileReferenceNumber = 0;
    mft_enum_data_v0.LowUsn = 0;
    mft_enum_data_v0.HighUsn = journal_data_v0.NextUsn;

    std::vector<BYTE> data(sizeof(DWORDLONG) + 0x10000);

	while (true) {
        if (!::DeviceIoControl(device.get(),
            FSCTL_ENUM_USN_DATA,
            &mft_enum_data_v0,
            sizeof(mft_enum_data_v0),
            data.data(),
            data.size(),
            &cb,
            nullptr)) {
            REPORT_ERROR("DeviceIoControl FSCTL_ENUM_USN_DATA");
            break;
        }

        auto* record = reinterpret_cast<PUSN_RECORD>(&data[sizeof(USN)]);
        while (reinterpret_cast<PBYTE>(record) < (data.data() + cb)) {
            std::wstring sz(reinterpret_cast<LPCWSTR>(reinterpret_cast<PBYTE>(record) + record->FileNameOffset),
                record->FileNameLength / sizeof(WCHAR));
        	
            auto fd = GetFileIdDescriptor(record->FileReferenceNumber);
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
            XAMP_LOG_DEBUG("Read file {}", ToString(file_path));
            record = reinterpret_cast<PUSN_RECORD>(reinterpret_cast<PBYTE>(record) + record->RecordLength);
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
