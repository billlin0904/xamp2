//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <deque>

#include <metadata/metadata.h>
#include <base/exception.h>
#include <base/memory.h>
#include <base/windows_handle.h>

namespace xamp::metadata::win32 {

inline constexpr std::string_view kNtfsSignature("NTFS    ");

#define NTFS_ATTR_MAX 16
#define	NTFS_ATTR_INDEX(at)	(((at)>>4)-1)
#define	NTFS_ATTR_MASK(at)	(((DWORD)1)<<NTFS_ATTR_INDEX(at))

#define	kNTFS_ATTR_STANDARD_INFORMATION	NTFS_ATTR_MASK(ATTR_TYPE_STANDARD_INFORMATION)
#define	kNTFS_ATTR_ATTRIBUTE_LIST		NTFS_ATTR_MASK(ATTR_TYPE_ATTRIBUTE_LIST)
#define	kNTFS_ATTR_FILE_NAME			NTFS_ATTR_MASK(ATTR_TYPE_FILE_NAME)
#define	kNTFS_ATTR_OBJECT_ID			NTFS_ATTR_MASK(ATTR_TYPE_OBJECT_ID)
#define	kNTFS_ATTR_SECURITY_DESCRIPTOR	NTFS_ATTR_MASK(ATTR_TYPE_SECURITY_DESCRIPTOR)
#define	kNTFS_ATTR_VOLUME_NAME			NTFS_ATTR_MASK(ATTR_TYPE_VOLUME_NAME)
#define	kNTFS_ATTR_VOLUME_INFORMATION	NTFS_ATTR_MASK(ATTR_TYPE_VOLUME_INFORMATION)
#define	kNTFS_ATTR_DATA					NTFS_ATTR_MASK(ATTR_TYPE_DATA)
#define	kNTFS_ATTR_INDEX_ROOT			NTFS_ATTR_MASK(ATTR_TYPE_INDEX_ROOT)
#define	kNTFS_ATTR_INDEX_ALLOCATION		NTFS_ATTR_MASK(ATTR_TYPE_INDEX_ALLOCATION)
#define	kNTFS_ATTR_BITMAP				NTFS_ATTR_MASK(ATTR_TYPE_BITMAP)
#define	kNTFS_ATTR_REPARSE_POINT		NTFS_ATTR_MASK(ATTR_TYPE_REPARSE_POINT)
#define	kNTFS_ATTR_EA_INFORMATION		NTFS_ATTR_MASK(ATTR_TYPE_EA_INFORMATION)
#define	kNTFS_ATTR_EA					NTFS_ATTR_MASK(ATTR_TYPE_EA)
#define	kNTFS_ATTR_LOGGED_UTILITY_STREAM NTFS_ATTR_MASK(ATTR_TYPE_LOGGED_UTILITY_STREAM)

// MFT Indexes
#define	NTFS_MFT_IDX_MFT			0
#define	NTFS_MFT_IDX_MFT_MIRR		1
#define	NTFS_MFT_IDX_LOG_FILE		2
#define	NTFS_MFT_IDX_VOLUME			3
#define	NTFS_MFT_IDX_ATTR_DEF		4
#define	NTFS_MFT_IDX_ROOT			5
#define	NTFS_MFT_IDX_BITMAP			6
#define	NTFS_MFT_IDX_BOOT			7
#define	NTFS_MFT_IDX_BAD_CLUSTER	8
#define	NTFS_MFT_IDX_SECURE			9
#define	NTFS_MFT_IDX_UPCASE			10
#define	NTFS_MFT_IDX_EXTEND			11
#define	NTFS_MFT_IDX_RESERVED12		12
#define	NTFS_MFT_IDX_RESERVED13		13
#define	NTFS_MFT_IDX_RESERVED14		14
#define	NTFS_MFT_IDX_RESERVED15		15
#define	NTFS_MFT_IDX_USER			16


#pragma pack(1)
struct NTFS_BPB {
	// jump instruction
	BYTE		Jmp[3];

	// signature
	BYTE		Signature[8];

	// BPB and extended BPB
	WORD		BytesPerSector;
	BYTE		SectorsPerCluster;
	WORD		ReservedSectors;
	BYTE		Zeros1[3];
	WORD		NotUsed1;
	BYTE		MediaDescriptor;
	WORD		Zeros2;
	WORD		SectorsPerTrack;
	WORD		NumberOfHeads;
	DWORD		HiddenSectors;
	DWORD		NotUsed2;
	DWORD		NotUsed3;
	ULONGLONG	TotalSectors;
	ULONGLONG	LCN_MFT;
	ULONGLONG	LCN_MFTMirr;
	DWORD		ClustersPerFileRecord;
	DWORD		ClustersPerIndexBlock;
	BYTE		VolumeSN[8];

	// boot code
	BYTE		Code[430];

	//0xAA55
	BYTE		_AA;
	BYTE		_55;
};
#pragma pack()

struct NTFS_ATTR_HEADER {
	DWORD Type;			// Attribute Type
	DWORD TotalSize;	// Length (including this header)
	BYTE  NonResident;	// 0 - resident, 1 - non resident
	BYTE  NameLength;	// name length in words
	WORD  NameOffset;	// offset to the name
	WORD  Flags;		// Flags
	WORD  Id;			// Attribute Id
};

struct NTFS_ATTR_RESIDENT {
	NTFS_ATTR_HEADER Header;	  // Common data structure
	DWORD			 AttrSize;	  // Length of the attribute body
	WORD			 AttrOffset;  // Offset to the Attribute
	BYTE			 IndexedFlag; // Indexed flag
	BYTE			 Padding;	  // Padding
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

#define	NTFS_FILE_RECORD_MAGIC		'ELIF'
#define	NTFS_FILE_RECORD_FLAG_INUSE	0x01	// File record is in use
#define	NTFS_FILE_RECORD_FLAG_DIR	0x02	// File record is a directory

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
}

class NTFSVolume : public std::enable_shared_from_this<NTFSVolume> {
public:
	NTFSVolume() = default;

	void Open(std::wstring const& volume);
	
	[[nodiscard]] uint64_t GetMTFAddress() const noexcept {
		return mft_addr_;
	}
private:
	void OpenVolume(std::wstring const& volume);

	size_t sector_size_ = 0;
	size_t cluster_size_ = 0;
	size_t file_record_size_ = 0;
	size_t index_block_size_ = 0;
	uint64_t mft_addr_ = 0;
	FileHandle volume_;
};

class NTFSAttribut;

class NTFSFileRecord : public std::enable_shared_from_this<NTFSFileRecord> {
public:
	explicit NTFSFileRecord(std::shared_ptr<NTFSVolume> volume) 
		: volume_(volume) {
		attrlist_.reserve(NTFS_ATTR_MAX);
	}

	[[nodiscard]] const NTFSAttribut* FindFirstAttr(DWORD attr_type) const {
		return nullptr;
	}

	[[nodiscard]] const NTFSAttribut* FindNextAttr(DWORD attr_type) const {
		return nullptr;
	}

	void SetAttrMask(DWORD mask) {
		attr_mask_ = mask | kNTFS_ATTR_STANDARD_INFORMATION | kNTFS_ATTR_ATTRIBUTE_LIST;
	}
private:
	std::shared_ptr<NTFSAttribut> Allocate(const NTFS_ATTR_HEADER* header);
	bool ParseAttrs(const NTFS_ATTR_HEADER* header);
	bool ParseFileRecord(ULONGLONG fileref);
	NTFS_FILE_RECORD_HEADER* ReadFileRecord(ULONGLONG& fileref);
	
	DWORD attr_mask_{ 0 };
	std::shared_ptr<NTFSVolume> volume_;
	std::vector<std::deque<std::shared_ptr<NTFSAttribut>>> attrlist_;
};

inline void NTFSVolume::OpenVolume(std::wstring const& volume) {
	volume_.reset(::CreateFileW(volume.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		nullptr));
	if (!volume_) {
		throw PlatformSpecException();
	}
	DWORD num = 0;
	NTFS_BPB bpb{};
	static_assert(sizeof(NTFS_BPB) > 512);
	if (::ReadFile(volume_.get(), &bpb, 512, &num, nullptr) && num == 512) {
		if (reinterpret_cast<const char*>(bpb.Signature) == kNtfsSignature) {
			sector_size_ = bpb.BytesPerSector;
			cluster_size_ = sector_size_ * bpb.SectorsPerCluster;

			auto sz = static_cast<char>(bpb.ClustersPerFileRecord);
			if (sz > 0)
				file_record_size_ = cluster_size_ * sz;
			else
				file_record_size_ = 1 << (-sz);
			sz = static_cast<char>(bpb.ClustersPerIndexBlock);
			if (sz > 0)
				index_block_size_ = cluster_size_ * sz;
			else
				index_block_size_ = 1 << (-sz);
			mft_addr_ = bpb.LCN_MFT * cluster_size_;
			return;
		}
	}
	throw PlatformSpecException();
}


class NTFSAttribut {
public:
	NTFSAttribut(const NTFS_ATTR_HEADER * header, std::shared_ptr<NTFSFileRecord> record) {
		header_ = header;
	}
private:
	const NTFS_ATTR_HEADER* header_;
};

class NTFSAttributNoResident : public NTFSAttribut {
public:
	NTFSAttributNoResident(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record) {

	}
};

class NTFSAttributResident : public NTFSAttribut {
public:
	NTFSAttributResident(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttribut(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_RESIDENT*>(header);
		body_ = reinterpret_cast<const void*>(reinterpret_cast<const BYTE*>(header_) + header_->AttrOffset);
		attr_size_ = header_->AttrSize;
	}

	DWORD GetAttrSize() const noexcept {
		return attr_size_;
	}

	bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) const noexcept {
		actural = 0;
		if (len == 0) {
			return true;
		}
		if (offset >= attr_size_) {
			return false;
		}
		DWORD offsetd = (DWORD)offset;
		if (offset + len >= attr_size_) {
			actural = attr_size_ - offsetd;
		}
		else {
			actural = len;
		}
		MemoryCopy(buf, (BYTE*)body_ + offsetd, actural);
		return true;
	}
private:
	const NTFS_ATTR_RESIDENT* header_;
	const void* body_;
	DWORD attr_size_;
};

class NTFSFileNameAttribut final : public NTFSAttributResident {
public:
	NTFSFileNameAttribut(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_FILE_NAME*>(header);
	}

	std::wstring_view GetFileName() const {
		if (header_->NameLength > 0) {
			return std::wstring_view(reinterpret_cast<const wchar_t *>(header_->Name), 
				header_->NameLength);
		}
		return L"";
	}
private:
	const NTFS_ATTR_FILE_NAME* header_;
};

void NTFSVolume::Open(std::wstring const& volume) {
	OpenVolume(volume);
	NTFSFileRecord record(enable_shared_from_this());
	record.SetAttrMask(kNTFS_ATTR_VOLUME_NAME | kNTFS_ATTR_VOLUME_INFORMATION);
}

std::shared_ptr<NTFSAttribut> NTFSFileRecord::Allocate(const NTFS_ATTR_HEADER* header) {
	switch (header->Type) {
	case kNTFS_ATTR_FILE_NAME:
		return std::make_shared<NTFSFileNameAttribut>(header, shared_from_this());
		break;
	}
	if (header->NonResident) {
		return std::make_shared<NTFSAttributNoResident>(header, shared_from_this());
	}
	return std::make_shared<NTFSAttributResident>(header, shared_from_this());
}

bool NTFSFileRecord::ParseAttrs(const NTFS_ATTR_HEADER* header) {
	DWORD attr_index = NTFS_ATTR_INDEX(header->Type);
	if (attr_index > NTFS_ATTR_MAX) {
		return false;
	}
	attrlist_[attr_index].push_back(Allocate(header));
	return true;
}

NTFS_FILE_RECORD_HEADER* NTFSFileRecord::ReadFileRecord(ULONGLONG& fileref) {
	return nullptr;
}

bool NTFSFileRecord::ParseFileRecord(ULONGLONG fileref) {
	auto header = ReadFileRecord(fileref);
	return false;
}

}
