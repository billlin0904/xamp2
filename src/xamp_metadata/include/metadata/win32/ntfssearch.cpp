#pragma comment(lib, "advapi32.lib")
#include <aclapi.h>
#include <base/scopeguard.h>
#include <base/logger.h>
#include <metadata/win32/ntfssearch.h>

namespace xamp::metadata::win32 {

#define	MFT_IDX_USER 16

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
inline constexpr DWORD kAttrTypeObjectId = 0x40;
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
inline constexpr DWORD kAttrTypeLoggedUtilityStream = 0x100;

enum {
	kNtfsAttrStandardInformation = NTFS_ATTR_MASK(kAttrTypeStandardInformation),
    kNtfsAttrAttributeList = NTFS_ATTR_MASK(kAttrTypeAttributeList),
    kNtfsAttrFileName = NTFS_ATTR_MASK(kAttrTypeFileName),
    kNtfsAttrObjectId = NTFS_ATTR_MASK(kAttrTypeObjectId),
    kNtfsAttrSecurityDescriptor = NTFS_ATTR_MASK(kAttrTypeSecurityDescriptor),
    kNtfsAttrVolumeName = NTFS_ATTR_MASK(kAttrTypeVolumeName),
    kNtfsAttrVolumeInformation = NTFS_ATTR_MASK(kAttrTypeVolumeInformation),
    kNtfsAttrData = NTFS_ATTR_MASK(kAttrTypeData),
    kNtfsAttrIndexRoot = NTFS_ATTR_MASK(kAttrTypeIndexRoot),
    kNtfsAttrIndexAllocation = NTFS_ATTR_MASK(kAttrTypeIndexAllocation),
    kNtfsAttrBitmap = NTFS_ATTR_MASK(kAttrTypeBitmap),
    kNtfsAttrReparsePoint = NTFS_ATTR_MASK(kAttrTypeReparsePoint),
    kNtfsAttrEaInformation = NTFS_ATTR_MASK(kAttrTypeEaInformation),
    kNtfsAttrEa = NTFS_ATTR_MASK(kAttrTypeEa),
    kNtfsAttrLoggedUtilityStream = NTFS_ATTR_MASK(kAttrTypeLoggedUtilityStream)
};

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

template <typename T>
static std::unique_ptr<T> MakeHeader(DWORD bufsz) {
	auto* buf = new BYTE[bufsz];
	return std::unique_ptr<T>(reinterpret_cast<T*>(buf));
}

void NTFSVolume::OpenVolume(std::wstring const& volume) {
	//=================================================
	// Create a well-known SID for the Everyone group.
	//=================================================

	SID_IDENTIFIER_AUTHORITY sid_auth_world = SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY sid_auth_nt = SECURITY_NT_AUTHORITY;
	PSID everyone_sid = nullptr;
	PSID admin_sid = nullptr;

	if (!::AllocateAndInitializeSid(&sid_auth_world, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&everyone_sid)) {
		throw PlatformSpecException();
	}

	XAMP_ON_SCOPE_EXIT({
		::FreeSid(everyone_sid);
		});

	EXPLICIT_ACCESS ea[2] = { 0 };
	ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = static_cast<LPTSTR>(everyone_sid);

	if (!::AllocateAndInitializeSid(&sid_auth_nt, 2,
		SECURITY_WORLD_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
		&admin_sid)) {
		throw PlatformSpecException();
	}

	XAMP_ON_SCOPE_EXIT({
		::FreeSid(admin_sid);
		});

	ea[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = static_cast<LPTSTR>(admin_sid);

	PACL ACL = nullptr;
	if (::SetEntriesInAcl(2, ea, nullptr, &ACL) != ERROR_SUCCESS) {
		throw PlatformSpecException();
	}

	XAMP_ON_SCOPE_EXIT({
		::LocalFree(ACL);
		});

	auto SD = ::LocalAlloc(LPTR,
	                      SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!SD) {
		throw PlatformSpecException();
	}

	XAMP_ON_SCOPE_EXIT({
		::LocalFree(SD);
		});

	if (!::InitializeSecurityDescriptor(SD,
		SECURITY_DESCRIPTOR_REVISION)) {
		throw PlatformSpecException();
	}
	if (!::SetSecurityDescriptorDacl(SD, TRUE, ACL, FALSE)) {
		throw PlatformSpecException();
	}
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = SD;
	sa.bInheritHandle = FALSE;
	
	volume_.reset(::CreateFileW(volume.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr));
	if (!volume_) {
		throw PlatformSpecException();
	}
	DWORD num = 0;
	NTFS_BPB bpb{};
	if (ReadFile(&bpb, 512, num) && num == 512) {
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

class NTFSAttributNoResident : public NTFSAttribut {
public:
	NTFSAttributNoResident(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttribut(header, record) {
	}

	bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) const noexcept override {
		return false;
	}
};

class NTFSAttributResident : public NTFSAttribut {
public:
	NTFSAttributResident(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttribut(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_RESIDENT*>(header);
		body_ = reinterpret_cast<const BYTE*>(header_) + header_->AttrOffset;
		attr_size_ = header_->AttrSize;
	}

	DWORD GetAttrSize() const noexcept {
		return attr_size_;
	}

	bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) const noexcept override {
		actural = 0;
		if (len == 0) {
			return true;
		}
		if (offset >= attr_size_) {
			return false;
		}
		const auto offsetd = static_cast<DWORD>(offset);
		if (offset + len >= attr_size_) {
			actural = attr_size_ - offsetd;
		}
		else {
			actural = len;
		}
		MemoryCopy(buf, body_ + offsetd, actural);
		return true;
	}
private:
	const NTFS_ATTR_RESIDENT* header_;
	const BYTE* body_;
	DWORD attr_size_;
};

class NTFSVolumeInformation final : public NTFSAttributResident {
public:
	NTFSVolumeInformation(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_VOLUME_INFORMATION*>(header);
	}

	WORD GetVersion() const noexcept {
		return MAKEWORD(header_->MinorVersion, header_->MajorVersion);
	}
private:
	const NTFS_ATTR_VOLUME_INFORMATION *header_;
};

class NTFSFileNameAttribut final : public NTFSAttributResident, public NTFSFileName {
public:
	NTFSFileNameAttribut(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record)
		, NTFSFileName(reinterpret_cast<const NTFS_ATTR_FILE_NAME*>(header)) {
	}
};

void NTFSVolume::Open(std::wstring const& volume) {
	OpenVolume(volume);
	auto record = MakeAlignedShared<NTFSFileRecord>(shared_from_this());
	record->SetAttrMask(kNtfsAttrVolumeName | kNtfsAttrVolumeInformation);
	if (!record->ParseFileRecord(kNtfsMftIdxVolume)) {
		throw PlatformSpecException();
	}
	record->ParseAttrs();
	auto volume_info = record->FindFirstAttr<NTFSVolumeInformation>(kNtfsAttrVolumeInformation);
	if (volume_info != nullptr) {
		XAMP_LOG_DEBUG("NTFS volume version: {}.{}",
			HIBYTE(volume_info->GetVersion()), LOBYTE(volume_info->GetVersion()));
	}
}

NTFSFileRecord::NTFSFileRecord(std::shared_ptr<NTFSVolume> volume)
	: volume_(volume) {
	attrlist_.resize(NTFS_ATTR_MAX);
}

void NTFSFileRecord::SetAttrMask(DWORD mask) {
	attr_mask_ = mask | kNtfsAttrStandardInformation | kNtfsAttrAttributeList;
}

bool NTFSFileRecord::FindSubEntry(std::wstring const& file_name, NTFSIndexEntry& entry) {
	return false;
}

std::shared_ptr<NTFSAttribut> NTFSFileRecord::FindFirstAttr(DWORD attr_type) {
	DWORD idx = NTFS_ATTR_INDEX(attr_type);
	return attrlist_[idx].FindFirst();
}

std::shared_ptr<NTFSAttribut> NTFSFileRecord::FindNextAttr(DWORD attr_type) {
	DWORD idx = NTFS_ATTR_INDEX(attr_type);
	return attrlist_[idx].FindNext();
}

std::shared_ptr<NTFSAttribut> NTFSFileRecord::Allocate(const NTFS_ATTR_HEADER* header) {
	switch (header->Type) {
	case kNtfsAttrFileName:
		return MakeAlignedShared<NTFSFileNameAttribut>(header, shared_from_this());
	case kNtfsAttrVolumeInformation:
		return MakeAlignedShared<NTFSVolumeInformation>(header, shared_from_this());
	}
	XAMP_LOG_DEBUG("Allocate ntfs type: {}", header->Type);
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
	attrlist_[attr_index].Insert(Allocate(header));
	return true;
}

std::unique_ptr<NTFS_FILE_RECORD_HEADER> NTFSFileRecord::ReadFileRecord(ULONGLONG& fileref) {
	DWORD readbytes = 0;
	if (fileref < MFT_IDX_USER || !volume_->GetMFTData()) {
		LARGE_INTEGER file_record_pos{};
		file_record_pos.QuadPart = volume_->GetMTFAddress() + (volume_->GetFileRecordSize() * fileref);
		file_record_pos.LowPart = volume_->SetFilePointer(file_record_pos);

		if (file_record_pos.LowPart == static_cast<DWORD>(-1) && ::GetLastError() != NO_ERROR) {
			return nullptr;
		}
		auto header = MakeHeader<NTFS_FILE_RECORD_HEADER>(volume_->GetFileRecordSize());
		
		if (volume_->ReadFile(header.get(), volume_->GetFileRecordSize(), readbytes)
			&& readbytes == volume_->GetFileRecordSize()) {
			return header;
		}
	} else {
		ULONGLONG file_record_pos = volume_->GetFileRecordSize() * fileref;
		auto header = MakeHeader<NTFS_FILE_RECORD_HEADER>(volume_->GetFileRecordSize());
		if (volume_->GetMFTData()->ReadData(file_record_pos, header.get(), volume_->GetFileRecordSize(), readbytes)
			&& readbytes == volume_->GetFileRecordSize()) {
			return header;
		}
	}
	return nullptr;
}

void NTFSFileRecord::PatchUS(WORD* sector, int sectors, WORD usn, WORD* usarray) {
	for (int i = 0; i < sectors; i++) {
		sector += ((volume_->GetSectorSize() >> 1) - 1);
		if (*sector != usn) {
			throw Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, "PathUS failure.");
		}
		*sector = usarray[i];
		sector++;
	}
}

void NTFSFileRecord::ParseAttrs() {
	auto* attr_header = reinterpret_cast<NTFS_ATTR_HEADER*>(reinterpret_cast<BYTE*>(file_record_header_.get())
		+ file_record_header_->OffsetOfAttr);
	DWORD data_ptr = file_record_header_->OffsetOfAttr;

	while (attr_header->Type != static_cast<DWORD>(-1)
		&& (data_ptr + attr_header->TotalSize) <= volume_->GetFileRecordSize()) {
		if (NTFS_ATTR_MASK(attr_header->Type) & attr_mask_) {
			if (!ParseAttrs(attr_header)) {
				throw Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, "ParseAttrs failure.");
			}
		}
		data_ptr += attr_header->TotalSize;
		attr_header = reinterpret_cast<NTFS_ATTR_HEADER*>(reinterpret_cast<BYTE*>(attr_header) + attr_header->TotalSize);
	}
}

bool NTFSFileRecord::ParseFileRecord(ULONGLONG fileref) {
	auto header = ReadFileRecord(fileref);
	if (!header) {
		return false;
	}

	if (header->Magic == kNtfsFileRecordMagic) {
		auto* usnaddr = reinterpret_cast<WORD*>(reinterpret_cast<BYTE*>(header.get()) + header->OffsetOfUS);
		auto usn = *usnaddr;
		auto* usarray = usnaddr + 1;
		PatchUS(reinterpret_cast<WORD*>(header.get()),
			volume_->GetFileRecordSize()
			/ volume_->GetSectorSize(), 
			usn, 
			usarray);
		file_record_header_ = std::move(header);
		return true;
	}
	return false;
}

}
