//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/metadata.h>
#include <base/exception.h>
#include <base/align_ptr.h>
#include <base/memory.h>
#include <base/logger.h>
#include <metadata/win32/ntfstypes.h>
#include <winioctl.h>

namespace xamp::metadata::win32 {

#define PointerToNext(t, p, v) ((t)(((PBYTE)p) + (v)))

template <typename T>
class SList {
public:
	using iterator = std::vector<std::shared_ptr<T>>::iterator;

	SList() = default;

	void Insert(std::shared_ptr<T> const& entry) {
		list_.push_back(entry);
		current_ = list_.begin();
	}

	std::shared_ptr<T> FindFirst() {
		current_ = list_.begin();
		if (current_ != list_.end()) {
			return *current_;
		}
		return nullptr;
	}

	std::shared_ptr<T> FindNext() {
		++current_;
		if (current_ != list_.end()) {
			return *current_;
		}
		return nullptr;
	}

	void Clear() {
		list_.clear();
		current_ = iterator();
	}
private:
	iterator current_;
	std::vector<std::shared_ptr<T>> list_;
};

template <typename T>
static std::unique_ptr<T> MakeUniqueHeader(DWORD bufsz) {
	auto* buf = new BYTE[bufsz];
	MemorySet(buf, 0, bufsz);
	assert(sizeof(T) <= bufsz);
	return std::unique_ptr<T>(reinterpret_cast<T*>(buf));
}

inline void PatchUS(WORD* sector, int sectors, WORD usn, WORD* usarray, DWORD sector_size) {
	XAMP_LOG_DEBUG("Patch US secotr:{} usn:{:#04x} sector_size:{}", sectors, usn, sector_size);

	for (int i = 0; i < sectors; i++) {
		sector += ((sector_size >> 1) - 1);
		if (*sector != usn) {
			throw LibrarySpecException("PathUS failure.");
		}
		*sector = usarray[i];
		sector++;
	}
}

class NTFSIndexBlock;
class NTFSFileRecord;

class NTFSFileName {
public:
	NTFSFileName() = default;

	explicit NTFSFileName(const NTFS_ATTR_FILE_NAME *header) {
		SetFileName(header);
	}

	void SetFileName(const NTFS_ATTR_FILE_NAME* header) {
		file_name_.assign(reinterpret_cast<const wchar_t*>(header->Name),
			header->NameLength);
	}

	std::wstring GetFileName() const {
		return file_name_;
	}
protected:
	std::wstring file_name_;
};

class NTFSAttribut;

class NTFSVolume : public std::enable_shared_from_this<NTFSVolume> {
public:
	NTFSVolume() = default;

	~NTFSVolume();

	void Open(std::wstring const& volume);

	[[nodiscard]] DWORD GetSectorSize() const noexcept {
		return sector_size_;
	}

	[[nodiscard]] DWORD GetClusterSize() const noexcept {
		return cluster_size_;
	}

	[[nodiscard]] DWORD GetIndexBlockSize() const noexcept {
		return index_block_size_;
	}

	[[nodiscard]] DWORD GetMTFAddress() const noexcept {
		return mft_addr_;
	}

	[[nodiscard]] DWORD GetFileRecordSize() const noexcept {
		return file_record_size_;
	}

	[[nodiscard]] std::shared_ptr<NTFSAttribut> GetMFTData() noexcept {
		return mft_data_;
	}

	BOOL DeviceIoControl(DWORD dwIoControlCode,
		LPVOID       lpInBuffer,
		DWORD        nInBufferSize,
		LPVOID       lpOutBuffer,
		DWORD        nOutBufferSize,
		LPDWORD      lpBytesReturned,
		LPOVERLAPPED lpOverlapped) noexcept {
		return ::DeviceIoControl(volume_.get(),
			dwIoControlCode,
			lpInBuffer, 
			nInBufferSize,
			lpOutBuffer,
			nOutBufferSize,
			lpBytesReturned,
			lpOverlapped);
	}

	BOOL ReadFile(void* buf, DWORD len, DWORD& actural) noexcept {
		return ::ReadFile(volume_.get(), buf, len, &actural, nullptr);
	}

	DWORD SetFilePointer(LARGE_INTEGER& pos, DWORD move_method) noexcept {
		return ::SetFilePointer(volume_.get(), pos.LowPart, &pos.HighPart, move_method);
	}

	std::wstring GetParentPath(PUSN_RECORD_V3 record) {
		wchar_t file_path[MAX_PATH]{};
		auto desc = GetFileIdDescriptor(record->FileReferenceNumber);
		FileHandle file(::OpenFileById(volume_.get(),
			&desc, 0, 0, nullptr, 0));
		::GetFinalPathNameByHandleW(file.get(), file_path, MAX_PATH, 0);
		return file_path;
	}
private:
	static FILE_ID_DESCRIPTOR GetFileIdDescriptor(const FILE_ID_128 fileId) {
		FILE_ID_DESCRIPTOR file_descriptor{};
		file_descriptor.Type = FileIdType;
		file_descriptor.ExtendedFileId = fileId;
		file_descriptor.dwSize = sizeof(file_descriptor);
		return file_descriptor;
	}

	void OpenVolume(std::wstring const& volume);

	DWORD sector_size_ = 0;
	DWORD cluster_size_ = 0;
	DWORD file_record_size_ = 0;
	DWORD index_block_size_ = 0;
	uint64_t mft_addr_ = 0;
	std::shared_ptr<NTFSAttribut> mft_data_;
	std::shared_ptr<NTFSFileRecord> mft_record_;
	FileHandle volume_;
};

class NTFSIndexEntry : public NTFSFileName {
public:
	NTFSIndexEntry() = delete;
	NTFSIndexEntry(const NTFSIndexEntry& entry) = delete;
	NTFSIndexEntry& operator=(const NTFSIndexEntry& entry) = delete;

	explicit NTFSIndexEntry(const NTFS_INDEX_ENTRY* entry) {
		if (entry == nullptr) {
			throw LibrarySpecException("entry can't be null.");
		}
		entry_ = entry;
		if (entry_->StreamSize > 0) {
			SetFileName(reinterpret_cast<const NTFS_ATTR_FILE_NAME*>(entry->Stream));
		}
	}

	[[nodiscard]] bool HasFileName() const noexcept {
		return entry_->StreamSize > 0;
	}

	[[nodiscard]] bool IsSubNodePtr() const noexcept {
		return entry_->Flags & INDEX_ENTRY_FLAG_SUBNODE;
	}

	[[nodiscard]] ULONGLONG GetSubNodeVCN() const noexcept {
		return *reinterpret_cast<const ULONGLONG*>(reinterpret_cast<const BYTE*>(entry_) + entry_->Size - 8);
	}

	[[nodiscard]] ULONGLONG GetFileReference() const noexcept {
		return entry_->FileReference & 0x0000FFFFFFFFFFFFUL;
	}
private:
	const NTFS_INDEX_ENTRY* entry_;
};

using NTFSIndexEntryList = SList<NTFSIndexEntry>;
using NTFSBlockEntry = std::pair<NTFSIndexBlock, std::shared_ptr<NTFSIndexEntry>>;

class XAMP_METADATA_API NTFSFileRecord : public std::enable_shared_from_this<NTFSFileRecord> {
public:
	explicit NTFSFileRecord(std::shared_ptr<NTFSVolume> volume);

	NTFSFileRecord() = default;

	void Open(std::wstring const& volume);

	[[nodiscard]] std::shared_ptr<NTFSAttribut> FindFirstAttr(DWORD attr_type);

	[[nodiscard]] std::shared_ptr<NTFSAttribut> FindNextAttr(DWORD attr_type);

	template <typename T>
	std::shared_ptr<T> FindFirstAttr(DWORD attr_type) {
		return std::dynamic_pointer_cast<T>(FindFirstAttr(attr_type));
	}

	template <typename T>
	std::shared_ptr<T> FindNextAttr(DWORD attr_type) {
		return std::dynamic_pointer_cast<T>(FindNextAttr(attr_type));
	}

	bool ParseFileRecord(ULONGLONG fileref);

	void SetAttrMask(DWORD mask = kNtfsAttrMaskAll);

	std::optional<NTFSBlockEntry> FindSubEntry(std::wstring const &file_name);

	std::shared_ptr<NTFSVolume> GetVolume() const {
		return volume_;
	}

	bool IsDirectory() const noexcept {
		return file_record_header_->Flags & kNtfsFileRecordFlagDir;
	}

	bool IsDeleted() const noexcept {
		return !(file_record_header_->Flags & kNtfsFileRecordFlagInuse);
	}

	void Traverse(std::function<void(std::shared_ptr<NTFSIndexEntry> const&)> const & callback);
private:
	void TraverseSubNode(const ULONGLONG& vcn, std::function<void(std::shared_ptr<NTFSIndexEntry> const&)> const& callback);
	void ParseAttrs();
	void ClearAttrs();
	std::shared_ptr<NTFSAttribut> Allocate(const NTFS_ATTR_HEADER* header);
	bool ParseAttrs(const NTFS_ATTR_HEADER* header);	
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> ReadFileRecord(ULONGLONG& fileref);
	std::optional<NTFSBlockEntry> VisitIndexBlock(const ULONGLONG& vcn, std::wstring const& file_name);

	DWORD attr_mask_{ kNtfsAttrMaskAll };
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> file_record_header_;
	std::shared_ptr<NTFSVolume> volume_;
	std::vector<SList<NTFSAttribut>> attrlist_;
};

class NTFSIndexBlock : public NTFSIndexEntryList {
public:
	NTFSIndexBlock() = default;

	std::unique_ptr<NTFS_INDEX_BLOCK>& Allocate(DWORD size) {
		Clear();
		index_block_.reset();
		index_block_ = MakeUniqueHeader<NTFS_INDEX_BLOCK>(size);
		return index_block_;
	}
private:
	std::unique_ptr<NTFS_INDEX_BLOCK> index_block_;
};

class NTFSAttribut {
public:
	virtual ~NTFSAttribut() = default;

	NTFSAttribut(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record) {
		record_ = record;
		header_ = header;
		sector_size_ = record_->GetVolume()->GetSectorSize();
		cluster_size_ = record_->GetVolume()->GetClusterSize();
		index_block_size_ = record_->GetVolume()->GetIndexBlockSize();
	}

	virtual bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) noexcept = 0;

protected:
	DWORD sector_size_ = 0;
	DWORD cluster_size_ = 0;
	DWORD index_block_size_ = 0;

	std::shared_ptr<NTFSFileRecord> record_;
private:
	const NTFS_ATTR_HEADER* header_;
};

}
