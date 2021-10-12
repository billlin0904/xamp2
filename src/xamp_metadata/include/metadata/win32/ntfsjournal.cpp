#include <base/buffer.h>
#include <metadata/win32/ntfssearch.h>
#include <metadata/win32/ntfsjournal.h>

namespace xamp::metadata::win32 {

NTFSJournal::NTFSJournal() 
	: volume_(std::make_shared<NTFSVolume>()) {
}

NTFSJournal::NTFSJournal(std::shared_ptr<NTFSVolume> volume) {
	volume_ = volume;
}

void NTFSJournal::Open(std::wstring const& volume) {
	volume_->Open(volume);
}

void NTFSJournal::Traverse(DWORDLONG fileref, std::function<void(std::wstring const&)> const& callback) {
	DWORD usn_data_size = 0;
	USN_JOURNAL_DATA usn_info{};
	volume_->DeviceIoControl(FSCTL_QUERY_USN_JOURNAL,
		nullptr,
		0,
		&usn_info,
		sizeof(usn_info),
		&usn_data_size,
		nullptr);

	MFT_ENUM_DATA med{};
	if (fileref != -1) {
		med.StartFileReferenceNumber = fileref;
	}
	med.MaxMajorVersion = usn_info.MaxSupportedMajorVersion;
	med.MinMajorVersion = usn_info.MinSupportedMajorVersion;
	med.LowUsn = 0;
	med.HighUsn = usn_info.NextUsn;

	Buffer<BYTE> buffer(sizeof(DWORDLONG) + 0x10000);
	PUSN_RECORD_V3 usn_record = nullptr;

	while (volume_->DeviceIoControl(FSCTL_ENUM_USN_DATA,
		&med,
		sizeof(med),
		buffer.data(),
		buffer.size(),
		&usn_data_size,
		nullptr)) {
		auto offset_bytes = usn_data_size - sizeof(USN);
		usn_record = PointerToNext(PUSN_RECORD_V3, buffer.data(), sizeof(USN));
		while (offset_bytes > 0) {
			if (usn_record->FileNameLength > 0) {
				std::wstring file_name(usn_record->FileName, usn_record->FileNameLength / 2);
				if (usn_record->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					callback(volume_->GetParentPath(usn_record) + L"\\");
				}
				else {
					callback(volume_->GetParentPath(usn_record) + L"\\" + file_name);
				}
			}
			auto record_len = usn_record->RecordLength;
			offset_bytes -= record_len;
			usn_record = PointerToNext(PUSN_RECORD_V3, usn_record, record_len);
		}
		if (fileref != -1) {
			break;
		}
		med.StartFileReferenceNumber = *(reinterpret_cast<USN*>(buffer.data()));
	}
}

void NTFSJournal::Traverse(std::function<void(std::wstring const&)> const& callback) {
	return Traverse(-1, callback);
}

}
