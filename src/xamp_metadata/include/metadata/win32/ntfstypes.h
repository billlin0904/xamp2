//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string_view>
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
inline constexpr DWORD kNtfsAttrMaskAll = 0xFFFFFFFF;

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
	ULONGLONG	MFTLogicalClusterNumber;
	ULONGLONG	MirrorLogicalClusterNumber;
	DWORD		ClustersPerFileRecord;
	DWORD		ClustersPerIndexBlock;
	ULONGLONG 	VolumeSN[8];

	// boot code
	BYTE		Code[430];

	//0x55AA
	BYTE		_55;
	BYTE		_AA;
};

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

struct NTFS_INDEX_ENTRY {
	ULONGLONG FileReference; // Low 6B: MFT record index, High 2B: MFT record sequence number
	WORD	  Size;			 // Length of the index entry
	WORD	  StreamSize;	 // Length of the stream
	BYTE	  Flags;		 // Flags
	BYTE	  Padding[3];	 // Padding
	BYTE	  Stream[1];	 // Stream
	// VCN of the sub node in Index Allocation, Offset = Size - 8
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

struct NTFS_ATTR_RESIDENT {
	NTFS_ATTR_HEADER Header;	  // Common data structure
	DWORD			 AttrSize;	  // Length of the attribute body
	WORD			 AttrOffset;  // Offset to the Attribute
	BYTE			 IndexedFlag; // Indexed flag
	BYTE			 Padding;	  // Padding
};

struct NTFS_ATTR_VOLUME_INFORMATION {
	BYTE Reserved1[8];	// Always 0 ?
	BYTE MajorVersion;	// Major version
	BYTE MinorVersion;	// Minor version
	WORD Flags;			// Flags
	BYTE Reserved2[4];	// Always 0 ?
};

struct NTFS_ATTR_INDEX_ROOT {
	// Index Root Header
	DWORD		AttrType;		// Attribute type (ATTR_TYPE_FILE_NAME: Directory, 0: Index View)
	DWORD		CollRule;		// Collation rule
	DWORD		IBSize;			// Size of index block
	BYTE		ClustersPerIB;	// Clusters per index block (same as BPB?)
	BYTE		Padding1[3];	// Padding
	// Index Header
	DWORD		EntryOffset;	// Offset to the first index entry, relative to this address(0x10)
	DWORD		TotalEntrySize;	// Total size of the index entries
	DWORD		AllocEntrySize;	// Allocated size of the index entries
	BYTE		Flags;			// Flags
	BYTE		Padding2[3];	// Padding
};

struct NTFS_ATTR_LIST {
	NTFS_ATTR_HEADER Header;		// Common data structure
	DWORD			 AttrType;		// Attribute Type
	WORD			 Length;		// Record length
	BYTE			 NameLength;	// Name length 
	BYTE			 NameOffset;	// Offset to Name
	ULONGLONG		 StartVCN;		// Starting VCN
	ULONGLONG		 FileReference; // Base File Reference of the attribute
	WORD			 Id;			// Attribute Id
	WORD			 Name[1];	    // Filename
};

#pragma pack()

}
