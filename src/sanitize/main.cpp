#include <iostream>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/asiodevicetype.h>
#include <metadata/win32/ntfssearch.h>
#include <player/audio_player.h>

#include "NTFS.h"

using namespace xamp;
using namespace player;
using namespace base;
using namespace output_device;
using namespace win32;
using namespace xamp::metadata::win32;

void TestPlayDSD() {
	auto player = MakeAlignedShared<AudioPlayer>();
	player->Open("C:\\Users\\rdbill0452\\Music\\Test\\DSD.dsf", ExclusiveWasapiDeviceType::Id);
	player->PrepareToPlay();
	player->Play();
	std::cin.get();
	player->Stop();
}

struct RawHandle {
	class CNTFSVolume* Volume;
	class CFileRecord* File;
	class CAttrBase* Data;
	uint64_t DataOffset;
};

bool GetRawFileByPath(CFileRecord* File, const char* Filename)
{
	Filename++;
	if (Filename[1] != ':') return false;
	if (Filename[2] != '\\') return false;

	File->SetAttrMask(MASK_INDEX_ROOT | MASK_INDEX_ALLOCATION);
	if (!File->ParseFileRecord(MFT_IDX_ROOT)) return false;
	if (!File->ParseAttrs()) return false;

	char* context = NULL;
	char path[32 * 1024]; // max per MSDN, but not really
	strncpy_s(path, Filename + 3, _TRUNCATE);
	for (char* token = strtok_s(path, "\\", &context); token; token = strtok_s(NULL, "\\", &context))
	{
		size_t bytes;
		CIndexEntry entry;
		wchar_t wide[MAX_PATH + 1];
		errno_t err = mbstowcs_s(&bytes, wide, sizeof(wide) / sizeof(wide[0]), token, _TRUNCATE);
		if (err) return false;
		if (!File->FindSubEntry(wide, entry)) return false;
		if (!File->ParseFileRecord(entry.GetFileReference())) return false;
		if (!File->ParseAttrs()) return false;
	}
	if (!File->IsDirectory())
	{
		File->SetAttrMask(MASK_ALL);
		if (!File->ParseAttrs()) fprintf(stderr, "failed MASK_ALL file reparse");
	}
	return true;
}

bool GetRawFileByPath(std::shared_ptr<NTFSFileRecord> File, const char* Filename)
{
	Filename++;
	if (Filename[1] != ':') return false;
	if (Filename[2] != '\\') return false;

	File->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	File->ParseFileRecord(kNtfsMftIdxRoot);
	File->ParseAttrs();

	char* context = NULL;
	char path[32 * 1024]; // max per MSDN, but not really
	strncpy_s(path, Filename + 3, _TRUNCATE);
	for (char* token = strtok_s(path, "\\", &context); token; token = strtok_s(NULL, "\\", &context))
	{
		size_t bytes;
		wchar_t wide[MAX_PATH + 1];
		errno_t err = mbstowcs_s(&bytes, wide, sizeof(wide) / sizeof(wide[0]), token, _TRUNCATE);
		if (err) return false;
		if (auto entry = File->FindSubEntry(wide)) {
			File->ParseFileRecord(entry.value().second->GetFileReference());
			File->ParseAttrs();
		}
		else {
			break;
		}
	}
	return true;
}

ULONGLONG GetFileReference(std::shared_ptr<NTFSFileRecord> record, std::wstring const &file_path) {
	record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	record->ParseFileRecord(kNtfsMftIdxRoot);
	record->ParseAttrs();

	auto sub_path = file_path.substr(3);
	for (const auto & path : String::Split(sub_path.c_str(), L"\\")) {
		if (auto entry = record->FindSubEntry(path.data())) {
			record->ParseFileRecord(entry.value().second->GetFileReference());
			record->ParseAttrs();
		}
	}
	return -1;
}

void TraverseSub(ULONGLONG fileref) {
	auto record = std::make_shared<NTFSFileRecord>();
	record->Open(L"C");
	record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	record->ParseFileRecord(fileref);
	record->ParseAttrs();
	record->Traverse([&](auto entry) {
		XAMP_LOG_DEBUG("Root file name: {}", String::ToString(entry->GetFileName()));
		TraverseSub(entry->GetFileReference());
		});
}

void Traverse(std::shared_ptr<NTFSFileRecord> record) {
	std::vector<ULONGLONG> filelist;

	record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	record->ParseFileRecord(kNtfsMftIdxRoot);
	record->ParseAttrs();

	record->Traverse([&](auto entry) {
		XAMP_LOG_DEBUG("Root file name: {}", String::ToString(entry->GetFileName()));
		filelist.push_back(entry->GetFileReference());
		});

	for (const auto & fileref: filelist) {
		TraverseSub(fileref);
	}	
}

void TestReadNTFSVolume() {
	auto file_record = std::make_shared<NTFSFileRecord>();
	//file_record->Open(L"C");
	//Traverse(file_record);
	//GetRawFileByPath(file_record, "‪C:\\Users\\bill\\Downloads\\basscd24.zip");
	file_record->Open(L"G");
	GetFileReference(file_record, L"G:\\Musics\\");
	CFileRecord record(new CNTFSVolume(L'C'));
	GetRawFileByPath(&record, "‪C:\\Users\\bill\\Downloads\\basscd24.zip");
}

int main() {	
	Logger::GetInstance()
		.AddDebugOutputLogger()
		.AddConsoleLogger()
		.AddFileLogger("xamp.log")
		.GetLogger("xamp");

	XAMP_SET_LOG_LEVEL(spdlog::level::debug);

	XAMP_ON_SCOPE_EXIT(
		Logger::GetInstance().Shutdown();
	);
	try
	{
		TestReadNTFSVolume();
	}catch (Exception const &e)
	{
		std::cout << e.GetErrorMessage();
		std::cin.get();
	}
	AudioPlayer::Initialize();
}
