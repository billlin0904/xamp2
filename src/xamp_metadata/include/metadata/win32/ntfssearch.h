//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/metadata.h>
#include <base/exception.h>
#include <base/windows_handle.h>

namespace xamp::metadata::win32 {

inline constexpr std::string_view kNtfsSignature("NTFS    ");

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

class NTFSVolume {
public:
	explicit NTFSVolume(std::wstring const & volume) {
		OpenVolume(volume);
	}

	void OpenVolume(std::wstring const& volume);

	[[nodiscard]] uint64_t GetMTFAddress() const noexcept {
		return mft_addr_;
	}
private:
	size_t sector_size_ = 0;
	size_t cluster_size_ = 0;
	size_t file_record_size_ = 0;
	size_t index_block_size_ = 0;
	uint64_t mft_addr_ = 0;
	FileHandle volume_;
};

class NTFSAttribut;

class NTFSFileRecord {
public:
	explicit NTFSFileRecord(std::shared_ptr<NTFSVolume> volume) {
	}

	[[nodiscard]] const NTFSAttribut* FindFirstAttr(DWORD attr_type) const {
		return nullptr;
	}

	[[nodiscard]] const NTFSAttribut* FindNextAttr(DWORD attr_type) const {
		return nullptr;
	}
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

class NTFSAttributResident : public NTFSAttribut {
public:
	NTFSAttributResident(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttribut(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_RESIDENT*>(header);
		body_ = reinterpret_cast<const void*>(reinterpret_cast<const BYTE*>(header_) + header_->AttrOffset);
		body_size_ = header_->AttrSize;
	}

private:
	const NTFS_ATTR_RESIDENT* header_;
	const void* body_;
	DWORD body_size_;
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
}
