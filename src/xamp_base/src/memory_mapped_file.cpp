#include <filesystem>

#ifdef _WIN32
#include <io.h>
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#include <base/unicode.h>
#endif

#include <base/exception.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/memory_mapped_file.h>

namespace xamp::base {

enum class FileAccessMode {
    READ,
    WRITE,
    READ_WRITE
};

#ifdef _WIN32
class MemoryMappedFile::MemoryMappedFileImpl {
public:
    static const DWORD DEFAULT_ACCESS_MODE = GENERIC_READ;
    static const DWORD DEFAULT_CREATE_TYPE = OPEN_EXISTING;
    static const DWORD DEFAULT_PROTECT = PAGE_READONLY;
    static const DWORD DEFAULT_ACCESS = FILE_MAP_READ;

    MemoryMappedFileImpl() {
    }

    void Open(const std::wstring& file_path, FileAccessMode mode, bool exclusive = false) {
        file_.reset(::CreateFileW(file_path.c_str(),
                                  DEFAULT_ACCESS_MODE,
                                  exclusive ? 0 : (FILE_SHARE_READ | (mode == FileAccessMode::READ_WRITE ? FILE_SHARE_WRITE : 0)),
                                  0,
                                  DEFAULT_CREATE_TYPE,
                                  FILE_FLAG_SEQUENTIAL_SCAN,
                                  0));

        if (file_) {
            OpenMappingFile(DEFAULT_PROTECT, DEFAULT_ACCESS);
            return;
        }

        throw FileNotFoundException();
    }

    void Open(FILE* file) {
        auto fd = fileno(file);
        auto osfhandle = (HANDLE)_get_osfhandle(fd);
        file_.reset(osfhandle);
        if (file_) {
            OpenMappingFile(DEFAULT_PROTECT, DEFAULT_ACCESS);
        }
    }

    void OpenMappingFile(DWORD protect, DWORD access) {
        const MappingFileHandle mapping_handle(::CreateFileMappingW(file_.get(), 0, protect, 0, 0, 0));
        if (mapping_handle) {
            address_.reset(::MapViewOfFile(mapping_handle.get(), access, 0, 0, 0));
        }
    }

    ~MemoryMappedFileImpl() {
        Close();
    }

    void Close() {
        address_.reset();
        file_.reset();
    }

    const void * GetData() const {
        return address_.get();
    }

    int64_t GetLength() const {
        LARGE_INTEGER li{};
        ::GetFileSizeEx(file_.get(), &li);
        return li.QuadPart;
    }
private:
    MappingAddressHandle address_;
    FileHandle file_;
};
#else
class MemoryMappedFile::MemoryMappedFileImpl {
public:
    MemoryMappedFileImpl()
        : mem_(nullptr) {
    }

    void Open(const std::wstring& file_path, FileAccessMode mode, bool exclusive = false) {
        file_.reset(open(ToUtf8String(file_path).c_str(), O_RDONLY));
        if (!file_) {
            throw FileNotFoundException();
        }
        mem_ = mmap(nullptr, GetLength(), PROT_READ, MAP_PRIVATE, file_.get(), 0);
        if (!mem_) {
            throw FileNotFoundException();
        }
    }

    void Open(FILE* file) {
    }

    ~MemoryMappedFileImpl() {
        Close();
    }

    void Close() {
        if (!mem_) {
            return;
        }
        munmap(mem_, GetLength());
        mem_ = nullptr;
    }

    void * GetData() const {
        return mem_;
    }

    size_t GetLength() const {
        struct stat file_info;
        file_info.st_size = 0;
        fstat(file_.get(), &file_info);
        return static_cast<size_t>(file_info.st_size);
    }
private:
    void *mem_;
    FileHandle file_;
};
#endif

MemoryMappedFile::MemoryMappedFile()
    : impl_(MakeAlign<MemoryMappedFileImpl>()) {
}

XAMP_PIMPL_IMPL(MemoryMappedFile)

void MemoryMappedFile::Open(FILE* file) {
    impl_->Open(file);
}

void MemoryMappedFile::Open(const std::wstring &file_path) {
    impl_->Open(file_path, FileAccessMode::READ);
}

void * MemoryMappedFile::GetData() const {
    return impl_->GetData();
}

size_t MemoryMappedFile::GetLength() const {
    return impl_->GetLength();
}

void MemoryMappedFile::Close() {
    impl_->Close();
}

}
