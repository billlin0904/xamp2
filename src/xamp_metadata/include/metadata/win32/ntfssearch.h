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
#include <base/logger.h>
#include <base/windows_handle.h>

namespace xamp::metadata::win32 {

#define NTFS_ATTR_MAX 16
#define	NTFS_ATTR_INDEX(at)	(((at)>>4)-1)
#define	NTFS_ATTR_MASK(at)	(((DWORD)1)<<NTFS_ATTR_INDEX(at))

inline constexpr std::string_view kNtfsSignature("NTFS    ");

// =============================================
// Attribut types
// =============================================
inline constexpr DWORD kAttrTypeStandardInformation = 0x10;
inline constexpr DWORD kAttrTypeAttributeList = 0x20;
inline constexpr DWORD kAttrTypeFileName = 0x30; 
inline constexpr DWORD kAttrTypeObjectId = 0x40; // NT/2K
inline constexpr DWORD kAttrTypeSecurityDescriptor = 0x50;
inline constexpr DWORD kAttrTypeVolumeName = 0x60;
inline constexpr DWORD kAttrTypeVolumeInformation = 0x70;
inline constexpr DWORD kAttrTypeData = 0x80;
inline constexpr DWORD kAttrTypeIndexRoot = 0x90;
inline constexpr DWORD kAttrTypeIndexAllocation = 0xA0;
inline constexpr DWORD kAttrTypeBitmap = 0xB0;
inline constexpr DWORD kAttrTypeReparsePoint = 0xC0;
inline constexpr DWORD kAttrTypeEaInformation = 0xD0;
inline constexpr DWORD kAttrTypeEa = 0xE0;
inline constexpr DWORD kAttrTypePropertySet = 0xF0;
inline constexpr DWORD kAttrTypeLoggedUtilityStream = 0x100;
inline constexpr DWORD kAttrTypeEnd = 0xFFFFFFFF;

inline constexpr DWORD kNtfsAttrMaskStandardInformation = NTFS_ATTR_MASK(kAttrTypeStandardInformation);
inline constexpr DWORD kNtfsAttrMaskAttributeList = NTFS_ATTR_MASK(kAttrTypeAttributeList);
inline constexpr DWORD kNtfsAttrMaskFileName = NTFS_ATTR_MASK(kAttrTypeFileName);
inline constexpr DWORD kNtfsAttrMaskObjectId = NTFS_ATTR_MASK(kAttrTypeObjectId);
inline constexpr DWORD kNtfsAttrMaskSecurityDescriptor = NTFS_ATTR_MASK(kAttrTypeSecurityDescriptor);
inline constexpr DWORD kNtfsAttrMaskVolumeName = NTFS_ATTR_MASK(kAttrTypeVolumeName);
inline constexpr DWORD kNtfsAttrMaskVolumeInformation = NTFS_ATTR_MASK(kAttrTypeVolumeInformation);
inline constexpr DWORD kNtfsAttrMaskData = NTFS_ATTR_MASK(kAttrTypeData);
inline constexpr DWORD kNtfsAttrMaskIndexRoot = NTFS_ATTR_MASK(kAttrTypeIndexRoot);
inline constexpr DWORD kNtfsAttrMaskIndexAllocation = NTFS_ATTR_MASK(kAttrTypeIndexAllocation);
inline constexpr DWORD kNtfsAttrMaskBitmap = NTFS_ATTR_MASK(kAttrTypeBitmap);
inline constexpr DWORD kNtfsAttrMaskReparsePoint = NTFS_ATTR_MASK(kAttrTypeReparsePoint);
inline constexpr DWORD kNtfsAttrMaskEaInformation = NTFS_ATTR_MASK(kAttrTypeEaInformation);
inline constexpr DWORD kNtfsAttrMaskEa = NTFS_ATTR_MASK(kAttrTypeEa);
inline constexpr DWORD kNtfsAttrMaskPropertySet = NTFS_ATTR_MASK(kAttrTypePropertySet);
inline constexpr DWORD kNtfsAttrMaskLoggedUtilityStream = NTFS_ATTR_MASK(kAttrTypeLoggedUtilityStream);

// =============================================
// MFT Indexes
// =============================================
inline constexpr DWORD kNtfsMftIdxMft = 0;
inline constexpr DWORD kNtfsMftIdxMftMirr = 1;
inline constexpr DWORD kNtfsMftIdxLogFile = 2;
inline constexpr DWORD kNtfsMftIdxVolume = 3;
inline constexpr DWORD kNtfsMftIdxAttrDef = 4;
inline constexpr DWORD kNtfsMftIdxRoot = 5;
inline constexpr DWORD kNtfsMftIdxBitmap = 6;
inline constexpr DWORD kNtfsMftIdxBoot = 7;
inline constexpr DWORD kNtfsMftIdxBadCluster = 8;
inline constexpr DWORD kNtfsMftIdxSecure = 9;
inline constexpr DWORD kNtfsMftIdxUpcase = 10;
inline constexpr DWORD kNtfsMftIdxExtend = 11;
inline constexpr DWORD kNtfsMftIdxReserved12 = 12;
inline constexpr DWORD kNtfsMftIdxReserved13 = 13;
inline constexpr DWORD kNtfsMftIdxReserved14 = 14;
inline constexpr DWORD kNtfsMftIdxReserved15 = 15;
inline constexpr DWORD kNtfsMftIdxUser = 16;

// =============================================
// File Record
// =============================================
inline constexpr DWORD kNtfsFileRecordMagic = 'ELIF';
inline constexpr DWORD kNtfsFileRecordFlagInuse = 0x01;	// File record is in use
inline constexpr DWORD kNtfsFileRecordFlagDir = 0x02;	// File record is a directory

#define	INDEX_ENTRY_FLAG_SUBNODE	0x01	// Index entry points to a sub-node
#define	INDEX_ENTRY_FLAG_LAST		0x02	// Last index entry in the node, no Stream

#define	INDEX_BLOCK_MAGIC 'XDNI'

#pragma pack(1)
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

struct NTFS_INDEX_ENTRY {
	ULONGLONG FileReference;	// Low 6B: MFT record index, High 2B: MFT record sequence number
	WORD	  Size;			// Length of the index entry
	WORD	  StreamSize;		// Length of the stream
	BYTE	  Flags;			// Flags
	BYTE	  Padding[3];		// Padding
	BYTE	  Stream[1];		// Stream
	// VCN of the sub node in Index Allocation, Offset = Size - 8
};

struct NTFS_ATTR_HEADER_NON_RESIDENT {
	NTFS_ATTR_HEADER Header;			// Common data structure
	ULONGLONG		 StartVCN;		// Starting VCN
	ULONGLONG		 LastVCN;		// Last VCN
	WORD			 DataRunOffset;	// Offset to the Data Runs
	WORD			 CompUnitSize;	// Compression unit size
	DWORD			 Padding;		// Padding
	ULONGLONG		 AllocSize;		// Allocated size of the attribute
	ULONGLONG		 RealSize;		// Real size of the attribute
	ULONGLONG		 IniSize;		// Initialized data size of the stream 
};

struct NTFS_DATARUN {
	LONGLONG  LCN;		// -1 to indicate sparse data
	ULONGLONG Clusters;
	ULONGLONG StartVCN;
	ULONGLONG LastVCN;
};

struct NTFS_INDEX_BLOCK {
	// Index Block Header
	DWORD		Magic;			// "INDX"
	WORD		OffsetOfUS;		// Offset of Update Sequence
	WORD		SizeOfUS;		// Size in words of Update Sequence Number & Array
	ULONGLONG	LSN;			// $LogFile Sequence Number
	ULONGLONG	VCN;			// VCN of this index block in the index allocation
	// Index Header
	DWORD		EntryOffset;	// Offset of the index entries, relative to this address(0x18)
	DWORD		TotalEntrySize;	// Total size of the index entries
	DWORD		AllocEntrySize;	// Allocated size of index entries
	BYTE		NotLeaf;		// 1 if not leaf node (has children)
	BYTE		Padding[3];		// Padding
};
#pragma pack()

template <typename T>
class SList {
public:
	using iterator = std::vector<std::shared_ptr<T>>::iterator;

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
		++current_;
		if (current_ != list_.end()) {
			return *current_;
		}
		return nullptr;
	}

	void Clear() {
		list_.clear();
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
	XAMP_LOG_TRACE("Patch US secotr:{} usn:{} sector_size:{}", sectors, usn, sector_size);

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

	BOOL ReadFile(void* buf, DWORD len, DWORD& actural) noexcept {
		return ::ReadFile(volume_.get(), buf, len, &actural, nullptr);
	}

	DWORD SetFilePointer(LARGE_INTEGER& pos, DWORD move_method) noexcept {
		return ::SetFilePointer(volume_.get(), pos.LowPart, &pos.HighPart, move_method);
	}
private:
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

	void ParseAttrs();

	void SetAttrMask(DWORD mask);

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

	void ClearAttrs();
	std::shared_ptr<NTFSAttribut> Allocate(const NTFS_ATTR_HEADER* header);
	bool ParseAttrs(const NTFS_ATTR_HEADER* header);	
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> ReadFileRecord(ULONGLONG& fileref);
	std::optional<NTFSBlockEntry> VisitIndexBlock(const ULONGLONG& vcn, std::wstring const& file_name);

	DWORD attr_mask_{ 0 };
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
