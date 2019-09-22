#include <filesystem>

#include <base/windows_handle.h>
#include <base/exception.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/file.h>

namespace xamp::base {

enum FileAccessMode {
	READ,
	WRITE,
	READ_WRITE
};

class MemoryMappedFile::MemoryMappedFileImpl {
public:
	MemoryMappedFileImpl() {
	}

	void Open(const std::wstring &file_path, FileAccessMode mode, bool exclusive = false) {
		static const DWORD access_mode = GENERIC_READ;
		static const DWORD create_type = OPEN_EXISTING;
		static const DWORD protect = PAGE_READONLY;
		static const DWORD access = FILE_MAP_READ;

		file_.reset(CreateFileW(file_path.c_str(),
			access_mode,
			exclusive ? 0 : (FILE_SHARE_READ | (mode == FileAccessMode::READ_WRITE ? FILE_SHARE_WRITE : 0)),
			0,
			create_type,
			FILE_FLAG_SEQUENTIAL_SCAN,
			0));

		if (file_) {
			const MappingFileHandle mapping_handle(CreateFileMappingW(file_.get(), 0, protect, 0, 0, 0));
			if (mapping_handle) {
				address_.reset(MapViewOfFile(mapping_handle.get(), access, 0, 0, 0));
				return;
			}
		}
		throw FileNotFoundException();
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
		GetFileSizeEx(file_.get(), &li);
		return li.QuadPart;
	}
private:
	MappingAddressHandle address_;
	FileHandle file_;	
};

MemoryMappedFile::MemoryMappedFile()
	: impl_(std::unique_ptr<MemoryMappedFileImpl>()) {
}

XAMP_PIMPL_IMPL(MemoryMappedFile)

void MemoryMappedFile::Open(const std::wstring &file_path) {
	impl_->Open(file_path, FileAccessMode::READ);
}

const void * MemoryMappedFile::GetData() const {
	return impl_->GetData();
}

int64_t MemoryMappedFile::GetLength() const {
	return impl_->GetLength();
}

void MemoryMappedFile::Close() {
	impl_->Close();
}

}
