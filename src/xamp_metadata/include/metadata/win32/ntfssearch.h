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

class NTFSFileRecord;

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

class XAMP_METADATA_API NTFSFileRecord : public std::enable_shared_from_this<NTFSFileRecord> {
public:
	explicit NTFSFileRecord(std::shared_ptr<NTFSVolume> volume);

	[[nodiscard]] const NTFSAttribut* FindFirstAttr(DWORD attr_type) const;

	[[nodiscard]] const NTFSAttribut* FindNextAttr(DWORD attr_type) const;
	bool ParseFileRecord(ULONGLONG fileref);
	void ParseAttrs();
	void SetAttrMask(DWORD mask);
private:
	std::shared_ptr<NTFSAttribut> Allocate(const NTFS_ATTR_HEADER* header);
	bool ParseAttrs(const NTFS_ATTR_HEADER* header);	
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> ReadFileRecord(ULONGLONG& fileref);
	void PatchUS(WORD* sector, int sectors, WORD usn, WORD* usarray);

	DWORD attr_mask_{ 0 };
	std::unique_ptr<NTFS_FILE_RECORD_HEADER> file_record_header_;
	std::shared_ptr<NTFSVolume> volume_;
	std::vector<std::deque<std::shared_ptr<NTFSAttribut>>> attrlist_;
};

}
