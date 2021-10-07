#pragma comment(lib, "advapi32.lib")
#include <aclapi.h>
#include <base/scopeguard.h>
#include <base/logger.h>
#include <metadata/win32/ntfssearch.h>

namespace xamp::metadata::win32 {

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

template <typename T>
static std::unique_ptr<T> MakeHeader(DWORD bufsz) {
	auto* buf = new BYTE[bufsz];
	MemorySet(buf, 0, bufsz);
	return std::unique_ptr<T>(reinterpret_cast<T*>(buf));
}

#define PointerToNext(t, p, v) ((t)(((PBYTE)p) + (v)))

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
			mft_addr_ = bpb.MFTLogicalClusterNumber * cluster_size_;
			return;
		}
	}
	throw PlatformSpecException();
}

class NTFSAttributNoResident : public NTFSAttribut {
public:
	NTFSAttributNoResident(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttribut(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_HEADER_NON_RESIDENT*>(header);
		buffer_.resize(cluster_size_);
		ParseDataRun();
	}

	ULONGLONG GetDataSize(ULONGLONG* alloc_size) const {
		if (alloc_size) {
			*alloc_size = header_->AllocSize;
		}
		return header_->RealSize;
	}

	void ParseDataRun() {
		const auto* data_run =
			reinterpret_cast<const BYTE*>(header_) + header_->DataRunOffset;

		while (*data_run) {
			LONGLONG length = 0;
			LONGLONG LCN_offset = 0;
			LONGLONG LCN = 0;
			ULONGLONG VCN = 0;

			if (PickData(&data_run, &length, &LCN_offset)) {
				LCN += LCN_offset;
				if (LCN < 0) {
					return;
				}

				XAMP_LOG_DEBUG("Data length = {} clusters, LCN = {}", length, LCN);
				XAMP_LOG_DEBUG(LCN_offset == 0 ? ", Sparse Data" : "");

				auto data_run = MakeAlignedShared<NTFS_DATARUN>();
				data_run->LCN = (LCN_offset == 0) ? -1 : LCN;
				data_run->Clusters = length;
				data_run->StartVCN = VCN;
				VCN += length;
				data_run->LastVCN = VCN - 1;

				if (data_run->LastVCN <= (data_run->LastVCN - data_run->StartVCN)) {
					datarun_list_.Insert(data_run);
				} else {
					throw LibrarySpecException("VCN exceeds bound.");
				}
			} else {
				break;
			}
		}
	}

	bool PickData(const BYTE** data_run, LONGLONG* length, LONGLONG* LCN_offset) {
		BYTE size = **data_run;
		int length_bytes = size & 0x0F;
		int offset_bytes = size >> 4;
		if (length_bytes > 8 || offset_bytes > 8) {
			return false;
		}

		*length = 0;
		MemoryCopy(length, *data_run, length_bytes);
		(*data_run) += length_bytes;
		*LCN_offset = 0;

		if (offset_bytes) {
			if ((*data_run)[offset_bytes - 1] & 0x80) {
				*LCN_offset = -1;
			}
			MemoryCopy(LCN_offset, *data_run, offset_bytes);
			(*data_run) += offset_bytes;
		}
		return true;
	}

	bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) const noexcept override {
		return false;
	}
private:
	const NTFS_ATTR_HEADER_NON_RESIDENT* header_;
	std::vector<BYTE> buffer_;
	SList<NTFS_DATARUN> datarun_list_;
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

class NTFSIndexRoot : public NTFSAttributResident, public NTFSIndexEntryList {
public:
	NTFSIndexRoot(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_INDEX_ROOT*>(header);
		if (IsFileName()) {
			ParseIndexEntries();
		}
	}

	bool IsFileName() const {
		return header_->AttrType == kNtfsAttrMaskFileName;
	}
private:
	void ParseIndexEntries() {
		auto* entry = reinterpret_cast<const NTFS_INDEX_ENTRY*>(
			reinterpret_cast<const BYTE*>(&(header_->EntryOffset)) + header_->EntryOffset);
		DWORD total_entry_size = entry->Size;
		while (total_entry_size <= header_->TotalEntrySize) {
			Insert(MakeAlignedShared<NTFSIndexEntry>(entry));
			if (entry->Flags & INDEX_ENTRY_FLAG_LAST) {
				break;
			}
			entry = reinterpret_cast<const NTFS_INDEX_ENTRY*>(
				reinterpret_cast<const BYTE*>(entry) + entry->Size);
			total_entry_size = entry->Size;
		}
	}

	const NTFS_ATTR_INDEX_ROOT* header_;
};

class NTFSIndexAlloc final : public NTFSAttributNoResident {
public:
	NTFSIndexAlloc(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributNoResident(header, record) {
		if (GetDataSize(nullptr) % index_block_size_) {
			return;
		}
		index_block_count_ = GetDataSize(nullptr) / index_block_size_;
	}

	ULONGLONG GetIndexBlockCount() const noexcept {
		return index_block_count_;
	}
private:
	ULONGLONG index_block_count_;
};

class NTFSVolumeInformation final : public NTFSAttributResident {
public:
	NTFSVolumeInformation(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_VOLUME_INFORMATION*>(header);
	}

	WORD GetMinorVersion() const noexcept {
		return header_->MinorVersion;
	}

	WORD GetMajorVersion() const noexcept {
		return header_->MajorVersion;
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

	mft_record_ = MakeAlignedShared<NTFSFileRecord>(shared_from_this());
	mft_record_->SetAttrMask(kNtfsAttrMaskVolumeName | kNtfsAttrMaskVolumeInformation | kNtfsAttrMaskData);

	if (!mft_record_->ParseFileRecord(kNtfsMftIdxVolume)) {
		throw PlatformSpecException();
	}

	mft_record_->ParseAttrs();
	mft_data_ = mft_record_->FindFirstAttr(kAttrTypeData);

	if (const auto volume_info = 
		mft_record_->FindFirstAttr<NTFSVolumeInformation>(kAttrTypeVolumeInformation)) {
		XAMP_LOG_DEBUG("NTFS volume version: {}.{}",
			volume_info->GetMajorVersion(), volume_info->GetMinorVersion());
	}
}

NTFSFileRecord::NTFSFileRecord(std::shared_ptr<NTFSVolume> volume)
	: volume_(volume) {
	attrlist_.resize(NTFS_ATTR_MAX);
}

void NTFSFileRecord::SetAttrMask(DWORD mask) {
	attr_mask_ = mask | kNtfsAttrMaskStandardInformation | kNtfsAttrMaskAttributeList;
}

std::optional<NTFSIndexEntry> NTFSFileRecord::FindSubEntry(std::wstring const& file_name) {
	const auto index_root = FindFirstAttr<NTFSIndexRoot>(kAttrTypeIndexRoot);
	if (!index_root) {
		return std::nullopt;
	}
	for (auto entry = index_root->FindFirst();
		; entry = index_root->FindNext()) {		
	}
	return std::nullopt;
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
	case kAttrTypeIndexAllocation:
		return MakeAlignedShared<NTFSIndexAlloc>(header, shared_from_this());
	case kAttrTypeIndexRoot:
		return MakeAlignedShared<NTFSIndexRoot>(header, shared_from_this());
	case kAttrTypeFileName:
		return MakeAlignedShared<NTFSFileNameAttribut>(header, shared_from_this());
	case kAttrTypeVolumeInformation:
		return MakeAlignedShared<NTFSVolumeInformation>(header, shared_from_this());
	}

	XAMP_LOG_DEBUG("Allocate ntfs attr type: {}", header->Type);

	if (header->NonResident) {
		return MakeAlignedShared<NTFSAttributNoResident>(header, shared_from_this());
	}
	return MakeAlignedShared<NTFSAttributResident>(header, shared_from_this());
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
			throw LibrarySpecException("PathUS failure.");
		}
		*sector = usarray[i];
		sector++;
	}
}

void NTFSFileRecord::ParseAttrs() {
	auto* attr_header = PointerToNext(NTFS_ATTR_HEADER *, file_record_header_.get(), file_record_header_->OffsetOfAttr);

	while (attr_header->Type != kAttrTypeEnd && attr_header->TotalSize > 0) {
		XAMP_LOG_DEBUG("Parse ntfs attr type: {}", attr_header->Type);
		if (NTFS_ATTR_MASK(attr_header->Type) & attr_mask_) {
			if (!ParseAttrs(attr_header)) {
				throw LibrarySpecException("ParseAttrs failure.");
			}
		}
		attr_header = PointerToNext(NTFS_ATTR_HEADER*, attr_header, attr_header->TotalSize);
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
