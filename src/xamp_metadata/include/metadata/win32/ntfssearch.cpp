#pragma comment(lib, "advapi32.lib")
#include <aclapi.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
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
#pragma pack()

#define PointerToNext(t, p, v) ((t)(((PBYTE)p) + (v)))

void NTFSVolume::OpenVolume(std::wstring const& volume) {
	//=================================================
	// Create a well-known SID for the Everyone group.
	//=================================================

	/*SID_IDENTIFIER_AUTHORITY sid_auth_world = SECURITY_WORLD_SID_AUTHORITY;
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
	sa.bInheritHandle = FALSE;*/
	
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
				XAMP_LOG_DEBUG(LCN_offset == 0 ? ", Sparse Data" : ", No Sparse Data");

				auto data_run = std::make_shared<NTFS_DATARUN>();
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
		(*data_run)++;

		const int length_bytes = size & 0x0F;
		const int offset_bytes = size >> 4;
		if (length_bytes > 8 || offset_bytes > 8) {
			return false;
		}

		*length = 0;
		MemoryCopy(length, *data_run, length_bytes);
		if (*length < 0) {
			return false;
		}

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

	bool ReadData(const ULONGLONG& offset, void* bufv, DWORD buf_len, DWORD& actural) noexcept override {
		if (offset > header_->RealSize) {
			return false;
		}
		if ((offset + buf_len) > header_->RealSize) {
			buf_len = static_cast<DWORD>(header_->RealSize - offset);
		}

		BYTE* buf = static_cast<BYTE*>(bufv);
		DWORD len = 0;

		ULONGLONG start_VCN = offset / cluster_size_;
		DWORD start_bytes = cluster_size_ - static_cast<DWORD>(offset % cluster_size_);

		if (start_bytes != cluster_size_) {
			XAMP_LOG_DEBUG("Start read cluster unaligned cluster");
			if (ReadVirtualClusters(start_VCN, 1, buffer_.data(), cluster_size_, len)
				&& len == cluster_size_) {
				len = (start_bytes < len) ? start_bytes : len;
				MemoryCopy(buf, buffer_.data() + cluster_size_ - start_bytes, len);
				buf += len;
				buf_len -= len;
				actural += len;
				start_VCN++;
			}
		}

		if (buf_len == 0) {
			XAMP_LOG_DEBUG("Read cluster unaligned cluster finished");
			return true;
		}

		DWORD aligned_clusters = buf_len / cluster_size_;
		if (aligned_clusters > 0) {
			XAMP_LOG_DEBUG("Start read cluster aligned cluster");
			DWORD aligned_size = aligned_clusters * cluster_size_;
			if (ReadVirtualClusters(start_VCN, aligned_clusters, buf, cluster_size_, len)
				&& len == cluster_size_) {
				start_VCN += aligned_clusters;
				buf += aligned_size;
				buf_len %= cluster_size_;
				actural += len;
				if (buf_len == 0) {
					XAMP_LOG_DEBUG("Read cluster aligned cluster finished");
					return true;
				}
			}
		}

		XAMP_LOG_DEBUG("Start read last cluster unaligned cluster");
		if (ReadVirtualClusters(start_VCN, 1, buffer_.data(), cluster_size_, len)
			&& len == cluster_size_) {
			XAMP_LOG_DEBUG("Start read last cluster unaligned cluster");
			MemoryCopy(buf, buffer_.data(), buf_len);
			actural = buf_len;
			return true;
		}
		return false;
	}
private:
	bool ReadVirtualClusters(ULONGLONG vcn, DWORD clusters, BYTE* buf, DWORD len, DWORD& actural) {
		// Verify if clusters exceeds DataRun bounds
		if (vcn + clusters > (header_->LastVCN - header_->StartVCN + 1)) {
			return false;
		}

		// Verify buffer size
		if (len < clusters * cluster_size_) {
			return false;
		}

		for (auto entry = datarun_list_.FindFirst()
			; entry != nullptr
			; entry = datarun_list_.FindNext()) {
			if (vcn >= entry->StartVCN && vcn <= entry->LastVCN) {
				DWORD clusters_read = 0;
				// Clusters from read pointer to the end
				ULONGLONG vcns = entry->LastVCN - vcn + 1;
				if (static_cast<ULONGLONG>(clusters) > vcns) {
					vcns = static_cast<DWORD>(vcns);
				} else {
					clusters_read = clusters;
				}
				if (ReadClusters(buf, clusters_read, entry->LCN + (vcn - entry->StartVCN))) {
					buf += clusters_read * cluster_size_;
					clusters -= clusters_read;
					actural += clusters_read;
					vcn += clusters_read;
				} else {
					break;
				}
				if (clusters == 0) {
					break;
				}
			}
		}
		actural *= cluster_size_;
		return true;
	}

	bool ReadClusters(void* buf, DWORD clusters, LONGLONG lcn) {
		if (lcn == -1) {
			MemorySet(buf, 0, clusters * cluster_size_);
			return true;
		}

		LARGE_INTEGER addr{};
		addr.QuadPart = lcn * cluster_size_;
		DWORD len = record_->GetVolume()->SetFilePointer(addr, FILE_BEGIN);

		if (len == static_cast<DWORD>(-1) && ::GetLastError() != NO_ERROR) {
			return false;
		}

		if (record_->GetVolume()->ReadFile(buf, clusters * cluster_size_, len)
			&& len == clusters * cluster_size_) {
			XAMP_LOG_DEBUG("Read clusters:{} lcn:{}", clusters, lcn);
			return true;
		}
		return false;
	}
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

	bool ReadData(const ULONGLONG& offset, void* buf, DWORD len, DWORD& actural) noexcept override {
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
protected:
	const NTFS_ATTR_RESIDENT* header_;
	const BYTE* body_;
	DWORD attr_size_;
};

class NTFSIndexRoot : public NTFSAttributResident, public NTFSIndexEntryList {
public:
	NTFSIndexRoot(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_INDEX_ROOT*>(body_);
		if (IsFileName()) {
			ParseIndexEntries();
		}
	}

	bool IsFileName() const {
		return header_->AttrType == kAttrTypeFileName;
	}
private:
	void ParseIndexEntries() {
		const auto* entry = PointerToNext(const NTFS_INDEX_ENTRY*,
			&header_->EntryOffset,
			header_->EntryOffset);
		DWORD total_entry_size = entry->Size;
		while (total_entry_size <= header_->TotalEntrySize) {
			Insert(std::make_shared<NTFSIndexEntry>(entry));
			if (entry->Flags & INDEX_ENTRY_FLAG_LAST) {
				break;
			}
			entry = PointerToNext(const NTFS_INDEX_ENTRY*, entry, entry->Size);
			total_entry_size += entry->Size;
		}
	}

	const NTFS_ATTR_INDEX_ROOT* header_;
};

class NTFSIndexAlloc final : public NTFSAttributNoResident {
public:
	NTFSIndexAlloc(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributNoResident(header, record) {
		if (GetDataSize(nullptr) % index_block_size_) {
			throw LibrarySpecException("Can't calculate number of index blocks.");
		}
		index_block_count_ = GetDataSize(nullptr) / index_block_size_;
	}

	ULONGLONG GetIndexBlockCount() const noexcept {
		return index_block_count_;
	}

	void ParseIndexBlock(const ULONGLONG& vcn, NTFSIndexBlock& block) {
		if (vcn >= GetIndexBlockCount()) {
			throw LibrarySpecException("Out of index blocks.");
		}

		XAMP_LOG_DEBUG("Read index block vcn: {}", vcn);

		auto& blocks = block.Alloc(index_block_size_);
		DWORD sectors = index_block_count_ / sector_size_;
		DWORD len = 0;

		if (ReadData(vcn * index_block_size_, blocks.get(), index_block_size_, len) &&
			len == index_block_size_) {
			if (blocks->Magic != INDEX_BLOCK_MAGIC) {
				throw LibrarySpecException("Index Block parse error: Magic mismatch");
			}

			WORD* usnaddr = PointerToNext(WORD*, blocks.get(), blocks->OffsetOfUS);
			WORD usn = *usnaddr;
			WORD* usarray = usnaddr + 1;
			PatchUS(reinterpret_cast<WORD*>(blocks.get()), sectors, usn, usarray, sector_size_);

			auto entry = PointerToNext(NTFS_INDEX_ENTRY*, &blocks->EntryOffset, blocks->EntryOffset);
			DWORD entry_size = entry->Size;

			while (entry_size <= blocks->TotalEntrySize) {
				auto index_entry = std::make_shared<NTFSIndexEntry>(entry);
				if (entry->Flags & INDEX_ENTRY_FLAG_LAST) {
					break;
				}
				block.Insert(index_entry);
				entry = PointerToNext(NTFS_INDEX_ENTRY*, entry, entry->Size);
				entry_size += entry->Size;
			}
		}
	}
private:
	DWORD index_block_count_;
};

class NTFSVolumeInformation final : public NTFSAttributResident {
public:
	NTFSVolumeInformation(const NTFS_ATTR_HEADER* header, std::shared_ptr<NTFSFileRecord> record)
		: NTFSAttributResident(header, record) {
		header_ = reinterpret_cast<const NTFS_ATTR_VOLUME_INFORMATION*>(body_);
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

void NTFSFileRecord::Open(std::wstring const& volume) {
	attrlist_.resize(NTFS_ATTR_MAX);
	volume_ = MakeAlignedShared<NTFSVolume>();
	volume_->Open(volume);
}

void NTFSFileRecord::SetAttrMask(DWORD mask) {
	attr_mask_ = mask | kNtfsAttrMaskStandardInformation | kNtfsAttrMaskAttributeList;
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
		return std::make_shared<NTFSIndexAlloc>(header, shared_from_this());
	case kAttrTypeIndexRoot:
		return std::make_shared<NTFSIndexRoot>(header, shared_from_this());
	case kAttrTypeFileName:
		return std::make_shared<NTFSFileNameAttribut>(header, shared_from_this());
	case kAttrTypeVolumeInformation:
		return std::make_shared<NTFSVolumeInformation>(header, shared_from_this());
	}

	XAMP_LOG_DEBUG("Allocate ntfs attr type: {}", header->Type);

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
		file_record_pos.LowPart = volume_->SetFilePointer(file_record_pos, FILE_BEGIN);

		if (file_record_pos.LowPart == static_cast<DWORD>(-1) && ::GetLastError() != NO_ERROR) {
			return nullptr;
		}
		auto header = MakeUniqueHeader<NTFS_FILE_RECORD_HEADER>(volume_->GetFileRecordSize());
		
		if (volume_->ReadFile(header.get(), volume_->GetFileRecordSize(), readbytes)
			&& readbytes == volume_->GetFileRecordSize()) {
			return header;
		}
	} else {
		ULONGLONG file_record_pos = volume_->GetFileRecordSize() * fileref;
		auto header = MakeUniqueHeader<NTFS_FILE_RECORD_HEADER>(volume_->GetFileRecordSize());
		if (volume_->GetMFTData()->ReadData(file_record_pos, header.get(), volume_->GetFileRecordSize(), readbytes)
			&& readbytes == volume_->GetFileRecordSize()) {
			return header;
		}
	}
	return nullptr;
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
			usarray,
			volume_->GetSectorSize());
		file_record_header_ = std::move(header);
		XAMP_LOG_DEBUG("Parse file record success");
		return true;
	}
	return false;
}

std::optional<std::shared_ptr<NTFSIndexEntry>> NTFSFileRecord::FindSubEntry(std::wstring const& file_name) {
	const auto index_root = FindFirstAttr<NTFSIndexRoot>(kAttrTypeIndexRoot);
	if (!index_root) {
		return std::nullopt;
	}
	for (auto entry = index_root->FindFirst();
		entry != nullptr; entry = index_root->FindNext()) {
		if (entry->HasFileName()) {
			XAMP_LOG_DEBUG("File name: {}", String::ToString(entry->GetFileName()));
			if (entry->GetFileName() == file_name) {
				return entry;
			}
		}
	}
	return std::nullopt;
}

void NTFSFileRecord::TraverseSubEntries(std::function<void(std::shared_ptr<NTFSIndexEntry>)> const& cb) {
	const auto index_root = FindFirstAttr<NTFSIndexRoot>(kAttrTypeIndexRoot);
	if (!index_root) {
		return;
	}
	if (!index_root->IsFileName()) {
		return;
	}
	for (auto entry = index_root->FindFirst();
		entry != nullptr; entry = index_root->FindNext()) {
		if (entry->HasFileName()) {
			XAMP_LOG_DEBUG("File name: {}", String::ToString(entry->GetFileName()));
		}
		if (entry->IsSubNodePtr()) {
			TraverseSubNode(entry->GetSubNodeVCN(), cb);
		}
	}
}

void NTFSFileRecord::TraverseSubNode(const ULONGLONG& vcn, std::function<void(std::shared_ptr<NTFSIndexEntry>)> const& cb) {
	const auto index_alloc = FindFirstAttr<NTFSIndexAlloc>(kAttrTypeIndexAllocation);
	if (!index_alloc) {
		return;
	}

	NTFSIndexBlock block;
	index_alloc->ParseIndexBlock(vcn, block);

	for (auto entry = block.FindFirst();
		entry != nullptr;
		entry = block.FindNext()) {
		if (entry->HasFileName()) {
			XAMP_LOG_DEBUG("SubNode File name: {}", String::ToString(entry->GetFileName()));
			cb(entry);
		}
		if (entry->IsSubNodePtr()) {
			TraverseSubNode(entry->GetSubNodeVCN(), cb);
		}
	}
}

}