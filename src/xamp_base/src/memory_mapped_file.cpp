#include <base/memory_mapped_file.h>

#include <base/base.h>
#include <base/platfrom_handle.h>
#include <base/exception.h>
#include <base/unique_handle.h>

#include <filesystem>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

class MemoryMappedFile::MemoryMappedFileImpl {
public:
    void Open(std::wstring const & file_path, bool is_module) {
        static constexpr DWORD kAccessMode = GENERIC_READ;
        static constexpr DWORD kCreateType = OPEN_EXISTING;
        static constexpr DWORD kAccess = FILE_MAP_READ;
        static constexpr DWORD kProtect = PAGE_READONLY;

        file_.reset(::CreateFileW(file_path.c_str(),
                                  kAccessMode,
                                  FILE_SHARE_READ,
                                  nullptr,
                                  kCreateType,
                                  FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr));

        if (file_) {
            OpenMappingFile(is_module ? (kProtect | SEC_IMAGE_NO_EXECUTE) : kProtect, kAccess);
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

    [[nodiscard]] void const * GetData() const noexcept {
        return address_.get();
    }

    [[nodiscard]] size_t GetLength() const {
        LARGE_INTEGER li{};
		::GetFileSizeEx(file_.get(), &li);
        return li.QuadPart;
    }
private:
    void OpenMappingFile(DWORD protect, DWORD access) {
        MappingFileHandle const mapping_handle(::CreateFileMapping(file_.get(),
            nullptr,
            protect,
            0, 
            0,
            nullptr));
        if (!mapping_handle) {
            throw PlatformException();
        }
        address_.reset(::MapViewOfFile(mapping_handle.get(),
            access,
            0,
            0,
            0));
        if (!address_) {
            throw PlatformException();
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

    void Open(std::wstring const& file_path, bool /*is_module*/) {
        file_.reset(::open(String::ToUtf8String(file_path).c_str(), O_RDONLY));
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
    : impl_(MakePimpl<MemoryMappedFileImpl>()) {
}

XAMP_PIMPL_IMPL(MemoryMappedFile)

void MemoryMappedFile::Open(std::wstring const &file_path, bool is_module) {
    impl_->Open(file_path, is_module);
}

void const * MemoryMappedFile::GetData() const noexcept {
    return impl_->GetData();
}

size_t MemoryMappedFile::GetLength() const {
    return impl_->GetLength();
}

void MemoryMappedFile::Close() noexcept {
    impl_->Close();
}

XAMP_BASE_NAMESPACE_END
