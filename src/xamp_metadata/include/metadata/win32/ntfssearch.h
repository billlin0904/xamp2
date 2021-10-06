//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <deque>

#include <metadata/metadata.h>
#include <base/exception.h>
#include <base/align_ptr.h>
#include <base/memory.h>
#include <base/windows_handle.h>

namespace xamp::metadata::win32 {

struct NTFS_FILE_RECORD_HEADER {
	DWORD		Magic;			// "FILE"
	WORD		OffsetOfUS;		// Offset of Update Sequence
	WORD		SizeOfUS;		// Size in words of Update Sequence Number & Array
	ULONGLONG	LSN;			// $LogFile Sequence Number
	WORD		SeqNo;			// Sequence number
	WORD		Hardlinks;		// Hard link count
	WORD		OffsetOfAttr;	// Offset of the first Attribute
	WORD		Flags;			// Flags
	DWORD		RealSize;		// Real size of the FILE record
	DWORD		AllocSize;		// Allocated size of the FILE record
	ULONGLONG	RefToBase;		// File reference to the base FILE record
	WORD		NextAttrId;		// Next Attribute Id
	WORD		Align;			// Align to 4 byte boundary
	DWORD		RecordNo;		// Number of this MFT Record
};

struct NTFS_ATTR_HEADER {
	DWORD Type;			// Attribute Type
	DWORD TotalSize;	// Length (including this header)
	BYTE  NonResident;	// 0 - resident, 1 - non resident
	BYTE  NameLength;	// name length in words
	WORD  NameOffset;	// offset to the name
	WORD  Flags;		// Flags
	WORD  Id;			// Attribute Id
};

struct NTFS_ATTR_FILE_NAME {
	ULONGLONG ParentRef;	// File reference to the parent directory
	ULONGLONG CreateTime;	// File creation time
	ULONGLONG AlterTime;	// File altered time
	ULONGLONG MFTTime;	    // MFT changed time
	ULONGLONG ReadTime;	    // File read time
	ULONGLONG AllocSize;	// Allocated size of the file
	ULONGLONG RealSize;  	// Real size of the file
	DWORD     Flags;		// Flags
	DWORD	  ER;			// Used by EAs and Reparse
	BYTE	  NameLength;	// Filename length in characters
	BYTE	  NameSpace;	// Filename space
	WORD	  Name[1];	    // Filename
};

#define	INDEX_ENTRY_FLAG_SUBNODE	0x01	// Index entry points to a sub-node
#define	INDEX_ENTRY_FLAG_LAST		0x02	// Last index entry in the node, no Stream

struct NTFS_INDEX_ENTRY {
	ULONGLONG	FileReference;	// Low 6B: MFT record index, High 2B: MFT record sequence number
	WORD		Size;			// Length of the index entry
	WORD		StreamSize;		// Length of the stream
	BYTE		Flags;			// Flags
	BYTE		Padding[3];		// Padding
	BYTE		Stream[1];		// Stream
	// VCN of the sub node in Index Allocation, Offset = Size - 8
};

template <typename T>
class SList {
public:
	using iterator = std::deque<std::shared_ptr<T>>::iterator;

	SList() {
	}

	void Insert(std::shared_ptr<T> const& entry) {
		list_.push_back(entry);
	}

	std::shared_ptr<T> FindFirst() {
		current_ = list_.begin();
		if (current_ != list_.end()) {
			return *current_;
		}
		return nullptr;
	}

	std::shared_ptr<T> FindNext() {
		if (current_ != list_.end()) {
			++current_;
			return *current_;
		}
		return nullptr;
	}
private:
	iterator current_;
	std::deque<std::shared_ptr<T>> list_;
};

class NTFSFileRecord;

class NTFSFileName {
public:
	NTFSFileName() {
	}

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

class XAMP_METADATA_API NTFSAttribut {
public:
	NTFSAttribut(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record) {
		header_ = header;
	}

	virtual bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) const noexcept = 0;

private:
	const NTFS_ATTR_HEADER* header_;
};

class XAMP_METADATA_API NTFSVolume : public std::enable_shared_from_this<NTFSVolume> {
public:
	NTFSVolume() = default;

	~NTFSVolume() = default;

	void Open(std::wstring const& volume);

	[[nodiscard]] DWORD GetSectorSize() const noexcept {
		return sector_size_;
	}

	[[nodiscard]] DWORD GetMTFAddress() const noexcept {
		return mft_addr_;
	}

	[[nodiscard]] DWORD GetFileRecordSize() const noexcept {
		return file_record_size_;
	}

	[[nodiscard]] NTFSAttribut* GetMFTData() noexcept {
		return mft_data_;
	}

	BOOL ReadFile(void* buf, DWORD len, DWORD& actural) noexcept {
		return ::ReadFile(volume_.get(), buf, len, &actural, nullptr);
	}

	DWORD SetFilePointer(LARGE_INTEGER& pos) noexcept {
		return ::SetFilePointer(volume_.get(), pos.LowPart, &pos.HighPart, FILE_BEGIN);
	}
private:
	void OpenVolume(std::wstring const& volume);

	DWORD sector_size_ = 0;
	DWORD cluster_size_ = 0;
	DWORD file_record_size_ = 0;
	DWORD index_block_size_ = 0;
	uint64_t mft_addr_ = 0;
	NTFSAttribut* mft_data_{ nullptr };
	FileHandle volume_;
};

class NTFSIndexEntry : public NTFSFileName {
public:
	NTFSIndexEntry()
		: entry_(nullptr) {
	}

	NTFSIndexEntry(const NTFSIndexEntry& entry) {
		*this = entry;
	}

	NTFSIndexEntry& operator=(const NTFSIndexEntry& entry) {
		if (this != &entry) {
			return *this;
		}
		if (!entry_) {
			return *this;
		}
		entry_ = (NTFS_INDEX_ENTRY*)(new BYTE[entry.entry_->Size]);
		MemoryCopy(entry_, entry.entry_, entry.entry_->Size);
		SetFileName((NTFS_ATTR_FILE_NAME*)entry_->Stream);
		return *this;
	}

	explicit NTFSIndexEntry(NTFS_INDEX_ENTRY* entry) {
		entry_ = entry;
		if (entry_->StreamSize > 0) {
			SetFileName((NTFS_ATTR_FILE_NAME*)(entry->Stream));
		}
	}

	bool IsSubNodePtr() const noexcept {
		if (entry_ != nullptr) {
			return entry_->Flags & INDEX_ENTRY_FLAG_SUBNODE;
		}
		return false;
	}

	ULONGLONG GetSubNodeVCN() const noexcept {
		if (entry_ != nullptr) {
			return *(ULONGLONG*)((BYTE*)entry_ + entry_->Size - 8);
		}
		return -1;
	}

	ULONGLONG GetFileReference() const noexcept {
		if (entry_ != nullptr) {
			return entry_->FileReference & 0x0000FFFFFFFFFFFFUL;
		}
		return -1;
	}
private:
	NTFS_INDEX_ENTRY* entry_;
};

class XAMP_METADATA_API NTFSFileRecord : public std::enable_shared_from_this<NTFSFileRecord> {
public:
	explicit NTFSFileRecord(std::shared_ptr<NTFSVolume> volume);

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

	void ParseAttrs();

	void SetAttrMask(DWORD mask);

	bool FindSubEntry(std::wstring const &file_name, NTFSIndexEntry &entry);
private:
	std::shared_ptr<NTFSAttribut> Allocate(const NTFS_ATTR_HEADER* header);
	bool ParseAttrs(const NTFS_ATTR_HEADER* header);	
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> ReadFileRecord(ULONGLONG& fileref);
	void PatchUS(WORD* sector, int sectors, WORD usn, WORD* usarray);

	DWORD attr_mask_{ 0 };
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> file_record_header_;
	std::shared_ptr<NTFSVolume> volume_;
	std::vector<SList<NTFSAttribut>> attrlist_;
};

}
