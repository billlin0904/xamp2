#include <base/base.h>
#include <filesystem>

#ifdef XAMP_OS_WIN
#include <io.h>
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#include <base/str_utilts.h>
#endif

#include <base/exception.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/memory_mapped_file.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN

class MemoryMappedFile::MemoryMappedFileImpl {
public:
    MemoryMappedFileImpl() noexcept {
    }

    void Open(std::wstring const & file_path) {
        constexpr DWORD kAccessMode = GENERIC_READ;
        constexpr DWORD kCreateType = OPEN_EXISTING;
        constexpr DWORD kProtect = PAGE_READONLY;
        constexpr DWORD kAccess = FILE_MAP_READ;

        file_.reset(::CreateFileW(file_path.c_str(),
                                  kAccessMode,
                                  FILE_SHARE_READ,
                                  0,
                                  kCreateType,
                                  FILE_FLAG_SEQUENTIAL_SCAN,
                                  0));

        if (file_) {
            OpenMappingFile(kProtect, kAccess);
            return;
        }

        throw FileNotFoundException();
    }

    ~MemoryMappedFileImpl() noexcept {
        Close();
    }

    void Close() noexcept {
        address_.reset();
        file_.reset();
    }

    void const * GetData() const noexcept {
        return address_.get();
    }

	size_t GetLength() const {
        LARGE_INTEGER li{};
		::GetFileSizeEx(file_.get(), &li);
        return li.QuadPart;
    }
private:
    void OpenMappingFile(DWORD protect, DWORD access) {
        MappingFileHandle const mapping_handle(::CreateFileMapping(file_.get(),
            0,
            protect,
            0, 
            0,
            nullptr));
        if (mapping_handle) {
            address_.reset(::MapViewOfFile(mapping_handle.get(), access, 0, 0, 0));
        }
        else {
            throw PlatformSpecException();
        }
    }

    MappingAddressHandle address_;
    FileHandle file_;
};
#else
class MemoryMappedFile::MemoryMappedFileImpl {
public:
    MemoryMappedFileImpl()
        : mem_(nullptr) {
    }

    void Open(std::wstring const& file_path) {
        file_.reset(::open(ToUtf8String(file_path).c_str(), O_RDONLY));
        if (!file_) {
            throw FileNotFoundException();
        }
        mem_ = ::mmap(nullptr, GetLength(), PROT_READ, MAP_PRIVATE, file_.get(), 0);
        if (!mem_) {
            throw FileNotFoundException();
        }
    }

    ~MemoryMappedFileImpl() noexcept {
        Close();
    }

    void Close() noexcept {
        if (!mem_) {
            return;
        }
        ::munmap(mem_, GetLength());
        mem_ = nullptr;
        file_.close();
    }

    void const * GetData() const noexcept {
        return mem_;
    }

    size_t GetLength() const {
        struct stat file_info;
        file_info.st_size = 0;
        ::fstat(file_.get(), &file_info);
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

void MemoryMappedFile::Open(std::wstring const &file_path) {
    impl_->Open(file_path);
}

void const * MemoryMappedFile::GetData() const {
    return impl_->GetData();
}

size_t MemoryMappedFile::GetLength() const {
    return impl_->GetLength();
}

void MemoryMappedFile::Close() noexcept {
    impl_->Close();
}

}
