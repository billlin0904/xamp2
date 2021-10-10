#include <iostream>
#include <base/scopeguard.h>
#include <base/threadpool.h>
#include <base/stopwatch.h>
#include <base/str_utilts.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/asiodevicetype.h>
#include <metadata/win32/ntfssearch.h>
#include <player/audio_player.h>

#include "NTFS.h"
#include <stack>

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
	//Filename++;
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
	//Filename++;
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

void TraverseSub(ULONGLONG fileref, std::wstring const &parent = L"") {
	auto record = std::make_shared<NTFSFileRecord>();
	record->Open(L"C");
	record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	record->ParseFileRecord(fileref);
	record->ParseAttrs();
	record->Traverse([&](auto entry) {
		XAMP_LOG_DEBUG("File name: {}\\{}", String::ToString(parent), String::ToString(entry->GetFileName()));
		TraverseSub(entry->GetFileReference(), parent + L"\\" + entry->GetFileName());
		});
}


void EnumFilesStd(std::wstring const& file_path) {
	Stopwatch sw;
	XAMP_LOG_DEBUG("Start enum file std");
	for (auto const& file_or_dir : RecursiveDirectoryIterator(Fs::absolute(file_path), kIteratorOptions)) {
		//XAMP_LOG_DEBUG("{}", file_or_dir.path().string());
	}
	XAMP_LOG_DEBUG("End enum file std {} sec", sw.ElapsedSeconds());
}

void EnumFiles(std::shared_ptr<NTFSFileRecord> record, std::wstring const &file_path) {
	Stopwatch sw;
	XAMP_LOG_DEBUG("Start enum file");

	record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	record->ParseFileRecord(kNtfsMftIdxRoot);
	record->ParseAttrs();

	ULONGLONG fileref = -1;
	auto sub_path = file_path.substr(3);

	if (!sub_path.empty()) {
		for (const auto& path : String::Split(sub_path.c_str(), L"\\")) {
			std::wstring spth(path.data(), path.length());
			if (auto entry = record->FindSubEntry(spth)) {
				record->ParseFileRecord(entry.value().second->GetFileReference());
				record->ParseAttrs();
				fileref = entry.value().second->GetFileReference();
			}
		}
	}
	else {
		fileref = kNtfsMftIdxRoot;
	}

	if (fileref != -1) {
		std::vector<ULONGLONG> filerefs;
		filerefs.reserve(64 * 1024);
		record->Traverse([&](auto entry) {
			//XAMP_LOG_DEBUG("{}", String::ToString(entry->GetFileName()));
			filerefs.push_back(entry->GetFileReference());
			});

		while (!filerefs.empty()) {	
			record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
			record->ParseFileRecord(filerefs.back());
			filerefs.pop_back();
			record->ParseAttrs();
			record->Traverse([&](auto entry) {
				//XAMP_LOG_DEBUG("{}", String::ToString(entry->GetFileName()));
				if (record->IsDirectory()) {
					filerefs.push_back(entry->GetFileReference());
				}
				});
		}
		
		/*
		BoundedQueue<ULONGLONG> queue(64 * 1024);
		std::vector<std::shared_ptr<NTFSFileRecord>> dispatch_context;
		ThreadPool threadpool;

		for (auto i = 0; i < threadpool.GetThreadCount(); ++i) {
			auto file_record = std::make_shared<NTFSFileRecord>();
			file_record->Open(L"C");
			dispatch_context.push_back(file_record);
		}

		record->Traverse([&](auto entry) {
			//XAMP_LOG_DEBUG("{}", String::ToString(entry->GetFileName()));
			queue.Enqueue(entry->GetFileReference());
			});

		std::vector<std::shared_future<void>> tasks;
		while (!queue.IsEmpty()) {
			ULONGLONG fileref = 0;
			if (!queue.TryDequeue(fileref)) {
				break;
			}

			auto task = threadpool.Spawn([fileref, &queue, &dispatch_context](int32_t thread_index) {				 
				auto record = dispatch_context[thread_index];
				record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
				record->ParseFileRecord(fileref);
				record->ParseAttrs();
				record->Traverse([&](auto entry) {
					if (record->IsDirectory()) {
						queue.Enqueue(entry->GetFileReference());
					}
					else {
						XAMP_LOG_DEBUG("{}", String::ToString(entry->GetFileName()));
					}
					});
				});

			tasks.push_back(task);
			if (tasks.size() == dispatch_context.size()) {
				for (auto& t : tasks) {
					t.get();
				}
				tasks.clear();
			}
		}
		*/
	}
	XAMP_LOG_DEBUG("End enum file {} sec", sw.ElapsedSeconds());
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
	//EnumFilesStd(L"C:\\Users\\bill\\Desktop\\source\\xamp2");
	EnumFilesStd(L"C:\\Qt\\");
	std::cin.get();
	auto file_record = std::make_shared<NTFSFileRecord>();
	file_record->Open(L"C");
	//Traverse(file_record);
	//GetRawFileByPath(file_record, "‪C:\\Users\\bill\\Downloads\\basscd24.zip");
	//file_record->Open(L"C");
	//EnumFiles(file_record, L"C:\\Users\\bill\\Pictures");
	//EnumFiles(file_record, L"C:\\Users\\bill\\Desktop\\source\\xamp2");
	EnumFiles(file_record, L"C:\\Qt\\");
	//file_record->Open(L"G");
	//EnumFiles(file_record, L"G:\\Musics\\");
	//file_record->Open(L"D");
	//EnumFiles(file_record, L"D:\\Games\\");
	//file_record->Open(L"E");
	//EnumFiles(file_record, L"E:\\Musics\\");
	//CFileRecord record(new CNTFSVolume(L'G'));
	//GetRawFileByPath(&record, "G:\\Musics\\");
	std::cin.get();
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
